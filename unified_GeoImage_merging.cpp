#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <regex>

#include <omp.h>
#include <mutex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include "datatype.h"

#define EXE_NAME "_unified_geoimage_merging"

// #define PRINT_DETAILS

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

/// 输入经纬度范围, 转换为OGRGeometry格式, 后面可计算交集
OGRGeometry* range_to_ogrgeometry(double lon_min, double lon_max, double lat_min, double lat_max);

struct ll_range{
    ll_range(double _lon_min, double _lon_max, double _lat_min, double _lat_max)
        :lon_min(_lon_min), lon_max(_lon_max), lat_min(_lat_min), lat_max(_lat_max){}
    double lon_min, lon_max, lat_min, lat_max;
};

enum class mergingMethod{minimum, maximum};

/// 该测试案例可成功通过 可用".*DEM.tif"来搜索后缀是DEM.tif的文件
int regex_test(); 
int extract_geometry_memory_test();

void print_imgpaths(string vec_name, vector<string>& paths);

int main(int argc, char* argv[])
{
    // return regex_test();
    // return extract_geometry_memory_test();

    argc = 6;
    argv = new char*[6];
    for(int i=0; i<6; i++){
        argv[i] = new char[256];
    }
    strcpy(argv[1], "E:\\DEM");
    // strcpy(argv[1], "D:\\1_Data\\shp_test\\TanDEM_DEM");
    strcpy(argv[2], "D:\\1_Data\\china_shp\\bou1_4p.shp");
    strcpy(argv[3], "0");
    strcpy(argv[4], ".*DEM.tif");
    strcpy(argv[5], "D:\\Copernicus_China_DEM_minimum.tif");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    if(argc < 5){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [dem folder] [target shp] [merging method] (regex str) [output tif]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, dem_folder.\n"
                " argv[2]: input, target shp.\n"
                " argv[3]: input, merging method, 0:minimum, 1:maximum.\n"
                " argv[4]: input, optional, regex string.\n"
                " argv[5]: output, merged dem tif.\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    bool b_regex = false;
    string regex_regular = "";
    string op_filepath = argv[4];
    if(argc > 5){
        b_regex = true;
        regex_regular = argv[4];
        op_filepath = argv[5];
    }

    mergingMethod merging_method = mergingMethod::minimum;
    {
        int temp = stoi(argv[3]);
        if(temp == 0)
            merging_method = mergingMethod::minimum;
        else
            merging_method = mergingMethod::maximum;
    }

    std::mutex mtx;



    /// 1.读取影像文件夹内所有影像信息待用, 并从中任选一个数据提取分辨率、坐标系统和数据类型信息。

    fs::path root_path(argv[1]);
    if(!fs::exists(root_path)){
        return return_msg(-4,"argv[1] is not existed.");
    }


    /// 通过正则表达式筛选的有效数据的地址
    vector<string> valid_imgpaths;
    /// 有效数据对应的四至范围
    vector<ll_range> valid_ll_ranges;
    int valid_num = 0;

    regex reg_f(regex_regular);
    smatch result;

    fs::path path_imgs_info(root_path.string() + "/IMGS_INFO_FOR_MERGING.txt");
    if(fs::exists(path_imgs_info))
    {
        spdlog::info("IMGS_INFO_FOR_MERGING.txt is existed");
        std::ifstream ifs(path_imgs_info.string());
        if(!ifs.is_open()){
            spdlog::warn("IMGS_INFO_FOR_MERGING.txt open failed, turn to write...");
            goto write;
        }
        string str;
        int idx = 0;
        vector<string> splited_str;
        while(getline(ifs,str))
        {
            idx++;
            if(idx == 1)
                continue;
            strSplit(str, splited_str, ",");
            if(splited_str.size()<5)
                continue;
            
            auto t = ll_range(stod(splited_str[1]),stod(splited_str[2]),stod(splited_str[3]),stod(splited_str[4]));

            valid_imgpaths.push_back(splited_str[0]);
            valid_ll_ranges.push_back(t);
            valid_num++;

            cout<<fmt::format("\rnumber of validimage: {}",valid_num);
        }
        ifs.close();
        cout<<"\n";
        spdlog::info("extract valid_imgpaths & valid_ll_ranges from IMGS_INFO_FOR_MERGING.txt");
    }
    else{
write:
        spdlog::info("there is no IMGS_INFO_FOR_MERGING.txt file, let's create it.(which will spend a little of times.)");
        std::ofstream ofs(path_imgs_info.string());
        if(ofs.is_open()){
            ofs<<"regex regular: "<<regex_regular<<endl;
        }
        int idx = 0;
        for (auto& iter : fs::recursive_directory_iterator(root_path)){
            if(iter.is_directory())
                continue;

            string filename = iter.path().filename().string();
            bool res = regex_match(filename, result, reg_f);
            if(b_regex && !res)
                continue;

            GDALDataset* ds = (GDALDataset*)GDALOpen(iter.path().string().c_str(), GA_ReadOnly);
            if(!ds){
                continue;
            }

            double* gt = new double[6];
            ds->GetGeoTransform(gt);
            int width = ds->GetRasterXSize();
            int height= ds->GetRasterYSize();

            GDALClose(ds);

            double lon_min = gt[0], lon_max = gt[0] +  width * gt[1];
            double lat_max = gt[3], lat_min = gt[3] + height * gt[5];
            auto t = ll_range(lon_min,lon_max,lat_min,lat_max);
            delete[] gt;
            
            
            valid_imgpaths.push_back(iter.path().string());
            valid_ll_ranges.push_back(t);
            valid_num++;
            cout<<fmt::format("\rnumber of validimage: {}",valid_num);

            if(ofs.is_open()){
                ofs<<fmt::format("{},{},{},{},{}",iter.path().string(),lon_min,lon_max,lat_min,lat_max)<<endl;
            }
        }
        cout<<"\n";
        spdlog::info("extract valid_imgpaths & valid_ll_ranges from dem file.");

        if(ofs.is_open()){
            ofs.close();
            spdlog::warn("valid_imgpaths & valid_ll_ranges has been write in IMGS_INFO_FOR_MERGING.txt file.");
        }
        else{
            spdlog::info("ofs.open IMGS_INFO_FOR_MERGING.txt failed, valid_imgpaths & valid_ll_ranges write in file failed.");
        }
        
    }

    spdlog::info(fmt::format("number of valid_file: {}\n",valid_imgpaths.size()));
    if(valid_imgpaths.size() == 0){
        return return_msg(-5, "there is no valid file in argv[1].");
    }
#ifdef PRINT_DETAILS
    print_imgpaths("valid_imgpaths",valid_imgpaths);
#endif
    system("pause");


    /// 1.2. 任选其一获取基本信息（分辨率、坐标系统、数据类型）, 用于创建输出文件
    double spacing = 0.;
    OGRSpatialReference* osr;
	
    GDALDataType datatype;
    int datasize = 0;
    {
        GDALDataset* temp_dataset;
        int i = 0;
        do{
            cout<<fmt::format("\r searching a valid dataset: id[{}], path:[{}]",i,valid_imgpaths[i]);
            temp_dataset = (GDALDataset*)GDALOpen(valid_imgpaths[i++].c_str(),GA_ReadOnly);
        }while(!temp_dataset || i > valid_imgpaths.size());

        GDALRasterBand* rb = temp_dataset->GetRasterBand(1);
        double gt[6];
        temp_dataset->GetGeoTransform(gt);
        spacing = gt[1];
        auto temp_osr = temp_dataset->GetSpatialRef();
        
        osr = temp_osr->CloneGeogCS();

        // auto epsg = temp_osr->GetAttrValue("AUTHORITY",1);
        // cout<<"AUTHORITY,1:" << epsg<<endl;
        // osr.importFromEPSG(atoi(epsg));
        
        datatype = rb->GetRasterDataType();
        GDALClose(temp_dataset);

        switch (datatype)
        {
        case GDT_Int16:
            datasize = 2;
            break;
        case GDT_Int32:
            datasize = 4;
            break;
        case GDT_Float32:
            datasize = 4;
            break;
        default:
            return return_msg(-5, "unsupported datatype.");
            break;
        }
    }
    cout<<"datatype is: "<<GDALGetDataTypeName(datatype)<<endl;
    cout<<"datasize is: "<<datasize<<endl;
    cout<<"osr.name is: "<<osr->GetName()<<endl;

    system("pause");


    /// 2. 从所有有效的影像文件中筛选出与shp有交集（根据相应的筛选策略min or max）的文件，并统计经纬度覆盖范围

    GDALDataset* shp_dataset = (GDALDataset*)GDALOpenEx(argv[2], GDAL_OF_VECTOR, NULL, NULL, NULL);
    if(!shp_dataset){
        return return_msg(-2,"invalid argv[2], open shp_dataset failed.");
    }
    OGRLayer* layer = shp_dataset->GetLayer(0);
    layer->ResetReading();

    // OGRFeature* feature;

    /// 如果merging_method 为 maximum, 则需要计算shp文件的四至范围
    OGREnvelope envelope_total;
    double shp_lon_min, shp_lon_max, shp_lat_min, shp_lat_max;
    if(layer->GetExtent(&envelope_total) == OGRERR_NONE){
        // success
        std::cout<<fmt::format("layer.range:left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.\n",
                envelope_total.MinX, envelope_total.MaxY, envelope_total.MaxX, envelope_total.MinY);
        shp_lon_min = envelope_total.MinX;
        shp_lon_max = envelope_total.MaxX;
        shp_lat_max = envelope_total.MaxY;
        shp_lat_min = envelope_total.MinY;
    }else{
        /// failure
        return return_msg(-3,"layer->GetExtent failed.");
    }

    
    /// 2.1 将所有shp里的geometry写到内存里, 方便后面频繁的提取
    vector<OGRGeometry*> shp_geometry_vec;
    auto destrory_geometrys = [&shp_geometry_vec](){
        for(auto& g: shp_geometry_vec){
            OGRGeometryFactory::destroyGeometry(g);
        }
    };
    bool feature_loop = false;
    layer->ResetReading();
    do{
        auto f = layer->GetNextFeature();
        if(f != NULL){
            auto g = f->GetGeometryRef();
            if (g){
                auto g2 = g->clone();
                shp_geometry_vec.push_back(g2);
            }
                
                // break;
        }
        else{
            break;
        }
        OGRFeature::DestroyFeature(f);
    } while (1);

    // spdlog::info("number of geometry in shp is: {}",shp_geometry_vec.size());
    // if(shp_geometry_vec.size() < 1){
    //     return return_msg(-3, "number of geometry in shp < 1");
    // }


    system("pause");

    double contains_lon_max = shp_lon_min, contains_lon_min = shp_lon_max;
    double contains_lat_max = shp_lat_min, contains_lat_min = shp_lat_max;

    int invalid_imgpath_num = 0; 
    int contains_num = 0;
    vector<string> contains_imgpath;
    
    // OGRGeometry* geometry;
    spdlog::info(fmt::format("merging method: {}", merging_method == mergingMethod::minimum ? "minimum" : "maximum"));

    int idx = 0;
    int last_percentage = -1;
#pragma omp parallel for
    for(int i=0; i<valid_imgpaths.size(); i++)
    // for(auto& imgpath : valid_imgpaths)
    {
        int current_percentage = idx * 1000 / valid_imgpaths.size();
        if(current_percentage > last_percentage){
            last_percentage = current_percentage;
            std::cout<<fmt::format("\r extract percentage {:.1f}%({}/{}): ",last_percentage/10., idx, valid_imgpaths.size());
        }
        idx++;

        string imgpath = valid_imgpaths[i];
        ll_range range = valid_ll_ranges[i];

        double img_lon_min = range.lon_min;
        double img_lon_max = range.lon_max;
        double img_lat_max = range.lat_max;
        double img_lat_min = range.lat_min;

        // GDALDataset* ds = (GDALDataset*)GDALOpen(imgpath.c_str(), GA_ReadOnly);
        // if(!ds){
        //     invalid_imgpath_num++;
        //     continue;
        // }

        // double* gt = new double[6];
        // ds->GetGeoTransform(gt);
        // int width = ds->GetRasterXSize();
        // int height= ds->GetRasterYSize();

        // GDALClose(ds);

        // double img_lon_min = gt[0], img_lon_max = gt[0] +  width * gt[1];
        // double img_lat_max = gt[3], img_lat_min = gt[3] + height * gt[5];

        // delete[] gt;
#ifdef PRINT_DETAILS
        std::cout<<fmt::format("img range, left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.",
                    img_lon_min, img_lat_max, img_lon_max, img_lat_min);
#endif
        /// 判断该影像与shp是否相交(广义上的相交，包含minimum和maximum两种相交方法)
        bool b_contains = false;
        if(merging_method == mergingMethod::minimum)
        {
            OGRGeometry* img_geometry = range_to_ogrgeometry(img_lon_min, img_lon_max, img_lat_min, img_lat_max);
            for(int g_idx = 0; g_idx < shp_geometry_vec.size(); g_idx++){
                if(shp_geometry_vec[g_idx]->Intersects(img_geometry)){
                    b_contains = true;
                    break;
                }
            }
            OGRGeometryFactory::destroyGeometry(img_geometry);

            // mtx.lock();
            // OGRGeometry* img_geometry = range_to_ogrgeometry(img_lon_min, img_lon_max, img_lat_min, img_lat_max);
            // // img_geometry->closeRings();
            // layer->ResetReading();
            
            // bool feature_loop = false;
            // do
            // {
            //     // cout<<xxx++<<endl;
            //     auto f = layer->GetNextFeature();
            //     if(f != NULL){
            //         auto g = f->GetGeometryRef();
            //         if (!g){
            //             ///Contains, Overlaps, Intersects
            //             if (g->Intersects(img_geometry)){
            //                 b_contains = true;
            //                 OGRGeometryFactory::destroyGeometry(g);
            //                 OGRFeature::DestroyFeature(f);
            //                 break;
            //             }
            //         }
            //         OGRGeometryFactory::destroyGeometry(g);
            //     }
            //     else{
            //         break;
            //     }
            //     OGRFeature::DestroyFeature(f);
            // } while (1);
            // mtx.unlock();

            // // while ((feature = layer->GetNextFeature()) != NULL){
            // //     geometry = feature->GetGeometryRef();
            //     // if (!geometry){
            //     //     ///Contains, Overlaps, Intersects
            //     //     if (geometry->Intersects(img_geometry)){
            //     //         b_contains = true;
            //     //         OGRGeometryFactory::destroyGeometry(geometry);
            //     //         break;
            //     //     }
            //     // }
            //     // OGRGeometryFactory::destroyGeometry(geometry);
            // // }
            // // OGRFeature::DestroyFeature(feature);
            // OGRGeometryFactory::destroyGeometry(img_geometry);
            
        }
        else{
            if(img_lon_min > shp_lon_max || img_lon_max < shp_lon_min || img_lat_max < shp_lat_min || img_lat_min > shp_lat_max){
                b_contains = false;
            }else{
                b_contains = true;
            }
        }
        
        
        if(!b_contains){
            continue;
        }

        /// 到这里说明相交
        mtx.lock();
        ++contains_num;

        contains_lon_max = MAX(img_lon_max, contains_lon_max);
        contains_lon_min = MIN(img_lon_min, contains_lon_min);
        contains_lat_max = MAX(img_lat_max, contains_lat_max);
        contains_lat_min = MIN(img_lat_min, contains_lat_min);
        contains_imgpath.push_back(imgpath);
        mtx.unlock();
    }
    cout<<"\n";
    GDALClose(shp_dataset);
    valid_imgpaths.clear();
    destrory_geometrys();

    spdlog::info(fmt::format("contains range, left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.\n",
                    contains_lon_min, contains_lat_max, contains_lon_max, contains_lat_min));

    spdlog::info(fmt::format("number of contains_image: {}",contains_num));

#ifdef PRINT_DETAILS
    print_imgpaths("contains_imgpath",contains_imgpath);
#endif

    if(contains_imgpath.size() < 1){
        return return_msg(-6, "contains_imgpath.size() < 1, there is no contained image.");
    }

    system("pause");

    /// 3. 读取影像文件, 将满足条件的所有影像（contains）写到同一个tif里

    int width  = ceil((contains_lon_max - contains_lon_min) / spacing);
    int height = ceil((contains_lat_max - contains_lat_min) / spacing);
    double op_gt[6] = {contains_lon_min, spacing, 0, contains_lat_max, 0, -spacing};

    spdlog::info(fmt::format("output_size: width:{}, height:{}",width, height));
    spdlog::info(fmt::format("output_geotranform: {},{},{},{},{},{}",
                    op_gt[0],op_gt[1],op_gt[2],op_gt[3],op_gt[4],op_gt[5]));

    GDALDriver* driver_tif = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* op_dataset = driver_tif->Create(op_filepath.c_str(), width, height, 1, datatype, NULL);
    if(!op_dataset){
        return return_msg(-6, "output filepath is invalid.");
    }
    GDALRasterBand* op_rb = op_dataset->GetRasterBand(1);
    op_dataset->SetGeoTransform(op_gt);
    if(op_dataset->SetSpatialRef(osr) != CE_None){
        spdlog::warn("op_dataset->SetSpatialRef(osr) failed.");
    }

    /// 3.1 输出影像的赋初始值
    spdlog::info(fmt::format("output_rasterband init...",width, height));
    idx = 0;
    last_percentage = -1;
    switch (datatype)
    {
    case GDT_Int16:
        {
            short* arr = new short[width];
            for(int i=0; i< width; i++) arr[i] = -32767;
            for(int i = 0; i< height; i++){
                int current_percentage = idx * 1000 / height;
                if(current_percentage > last_percentage){
                    last_percentage = current_percentage;
                    std::cout<<fmt::format("\r extract percentage {:.1f}%({}/{}): ",last_percentage/10., idx, height);
                }
                idx++;
                op_rb->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            }
            delete[] arr;
            op_rb->SetNoDataValue(-32767);
        }
        break;
    case GDT_Int32:
        {
            int* arr = new int[width];
            for(int i=0; i< width; i++) arr[i] = -32767;
            for(int i = 0; i< height; i++){
                int current_percentage = idx * 1000 / height;
                if(current_percentage > last_percentage){
                    last_percentage = current_percentage;
                    std::cout<<fmt::format("\r extract percentage {:.1f}%({}/{}): ",last_percentage/10., idx, height);
                }
                idx++;
                op_rb->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            }
            delete[] arr;
            op_rb->SetNoDataValue(-32767);
        }
        break;
    case GDT_Float32:
        {
            float* arr = new float[width];
            for(int i=0; i< width; i++) arr[i] = NAN;
            for(int i = 0; i< height; i++){
                int current_percentage = idx * 1000 / height;
                if(current_percentage > last_percentage){
                    last_percentage = current_percentage;
                    std::cout<<fmt::format("\r extract percentage {:.1f}%({}/{}): ",last_percentage/10., idx, height);
                }
                idx++;
                op_rb->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            }
            delete[] arr;
            op_rb->SetNoDataValue(NAN);
        }
        break;
    }

    system("pause");

    /// 3.2 拼接
    idx = 0;
    last_percentage = -1;
#pragma omp parallel for
    for(int i = 0; i< contains_imgpath.size(); i++)
    // for(auto& imgpath : contains_imgpath)
    {
        int current_percentage = idx * 1000 / contains_num;
        if(current_percentage > last_percentage){
            last_percentage = current_percentage;
            std::cout<<fmt::format("\r merging percentage {:.1f}%({}/{}): ",last_percentage/10., idx, contains_num);
        }
        idx++;

        string imgpath = contains_imgpath[i];
        GDALDataset* ds = (GDALDataset*)GDALOpen(imgpath.c_str(), GA_ReadOnly);
        if(!ds){
            continue;
        }
        GDALRasterBand* rb = ds->GetRasterBand(1);

        int tmp_width = ds->GetRasterXSize();
        int tmp_height= ds->GetRasterYSize();
        double tmp_gt[6];
        ds->GetGeoTransform(tmp_gt);

        int start_x = round((tmp_gt[0] - op_gt[0]) / op_gt[1]);
        int start_y = round((tmp_gt[3] - op_gt[3]) / op_gt[5]);
#ifdef PRINT_DETAILS
        std::cout<<fmt::format("img in root: start({},{}), size:({},{}), end:({},{})\n\tpath:[{}]",
                        start_x,start_y,
                        tmp_width, tmp_height,
                        start_x + tmp_width - 1,
                        start_y + tmp_height - 1,
                        imgpath);
#endif
        void* arr = malloc(tmp_width * tmp_height * datasize);
        
        rb->RasterIO(GF_Read, 0, 0, tmp_width, tmp_height, arr, tmp_width, tmp_height, datatype, 0, 0);
        mtx.lock();
        op_rb->RasterIO(GF_Write, start_x, start_y, tmp_width, tmp_height, arr, tmp_width, tmp_height, datatype, 0, 0);
        mtx.unlock();

        free(arr);
        GDALClose(ds);
    }
    std::cout<<"\n";

    GDALClose(op_dataset);
    
    return return_msg(1, EXE_NAME " end.");
}


OGRGeometry* range_to_ogrgeometry(double lon_min, double lon_max, double lat_min, double lat_max)
{
    OGRPolygon* polygen = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
    OGRLinearRing* ring = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
    OGRPoint point;

    /// topleft
    point.setX(lon_min); point.setY(lat_max);
    ring->addPoint(&point);

    /// topright
    point.setX(lon_max); point.setY(lat_max);
    ring->addPoint(&point);

    /// downright
    point.setX(lon_max); point.setY(lat_min);
    ring->addPoint(&point);

    /// downleft
    point.setX(lon_min); point.setY(lat_min);
    ring->addPoint(&point);

    /// topleft
    point.setX(lon_min); point.setY(lat_max);
    ring->addPoint(&point);

    ring->closeRings();
    polygen->addRing(ring);

    OGRGeometry* geometry = (OGRGeometry*)polygen;
    return geometry;
}

int regex_test()
{
    string regex_str = ".*DEM.tif$";
    regex reg_f(regex_str);
    smatch result;
    fs::path p_folder("D:\\1_Data\\Task-3811701\\TanDEM_DEM");
    for(auto& iter : fs::recursive_directory_iterator(p_folder))
    {
        fs::path filepath = iter.path();
        string filename = filepath.filename().string();
        bool res = regex_match(filename, result, reg_f);
        if(res){
            std::cout<<fmt::format("filename [{}] conforms to regex [{}]\n",filename, regex_str);
        }
    }
    return 1;
}

int extract_geometry_memory_test()
{
    int i=0;
    int total = 100000;
    while(++i < total)
    {
        if(i % 100 == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        cout<<fmt::format("\rextract_geometry_memory_test, percentage: {}/{}",i,total);
        auto geometry = range_to_ogrgeometry(1,2,3,4);
        // OGRGeometryFactory::destroyGeometry(geometry);
    }
    return 1;
}


void print_imgpaths(string vec_name, vector<string>& paths)
{
    std::cout<<"print "<<vec_name<<":\n";
    int index = 0;
    for(auto& it : paths){
        cout<<fmt::format("id[{:>2}] {}\n",index, it);
        index++;
    }
    cout<<endl;
}