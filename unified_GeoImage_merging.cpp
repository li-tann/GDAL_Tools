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

#define EXE_NAME "unified_geoimage_merging"

// #define PRINT_DETAILS
// #define SYSTEM_PAUSE

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

// enum class mergingMethod{minimum, maximum};
enum class mergingMethod{rectangle, intersect_rectangle, irregular};

/// 该测试案例可成功通过 可用".*DEM.tif"来搜索后缀是DEM.tif的文件
int regex_test(); 
int extract_geometry_memory_test();

void print_imgpaths(string vec_name, vector<string>& paths);

int main(int argc, char* argv[])
{
    // return regex_test();
    // return extract_geometry_memory_test();

    // argc = 7;
    // argv = new char*[7];
    // for(int i=0; i<7; i++){
    //     argv[i] = new char[256];
    // }
    // strcpy(argv[1], "E:\\DEM");
    // // strcpy(argv[1], "D:\\1_Data\\shp_test\\TanDEM_DEM");
    // strcpy(argv[2], "D:\\1_Data\\china_shp\\bou1_4p.shp");
    // strcpy(argv[3], "0");
    // strcpy(argv[4], "0.1");
    // strcpy(argv[5], ".*DEM.tif");
    // strcpy(argv[6], "D:\\Copernicus_China_DEM_minimum.tif");

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
                " argv[3]: input, merging method, 0:rectangle, 1:intersect-rectangle, 2:irregular.\n"
                " argv[4]: input, shp buffer dist, ref: 0.01 (if unit is degree) or 1000 (if unit is meter)...\n"
                " argv[5]: input, optional, regex string.\n"
                " argv[6]: output, merged dem tif.\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    bool b_regex = false;
    string regex_regular = "";
    string op_filepath = argv[5];
    if(argc > 5){
        b_regex = true;
        regex_regular = argv[5];
        op_filepath = argv[6];
    }

    mergingMethod merging_method = mergingMethod::intersect_rectangle;  // 图像挑选策略
    {
        int temp = stoi(argv[3]);
        if(temp == 0)
            merging_method = mergingMethod::rectangle;
        else if(temp == 1)
            merging_method = mergingMethod::intersect_rectangle;
        else
            merging_method = mergingMethod::irregular;
    }

    double buffer_dist = stod(argv[4]);

    std::mutex mtx; // 多线程并行锁



    /// 1.读取影像文件夹内所有影像信息待用, 并从中任选一个数据提取分辨率、坐标系统和数据类型信息。
    spdlog::info(" #1. Read all the image information in the DEM Folder for use. (it may take a long time for the first time)");

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
        /// 如果IMGS_INFO_FOR_MERGING.txt文件已存在, 就直接读取文件内的DEM地址以及DEM对应的四至范围
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
        /// 如果找不到IMGS_INFO_FOR_MERGING.txt文件, 就逐个读取并存储
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
            spdlog::info(fmt::format("\rnumber of validimage: {}",valid_num));

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

    spdlog::info(fmt::format("number of valid_file: {}",valid_imgpaths.size()));
    if(valid_imgpaths.size() == 0){
        return return_msg(-5, "there is no valid file in argv[1].");
    }
#ifdef PRINT_DETAILS
    print_imgpaths("valid_imgpaths",valid_imgpaths);
#endif

#ifdef SYSTEM_PAUSE
    system("pause");
#endif
    

    /// 1.2. 任选其一获取基本信息（分辨率、坐标系统、数据类型）, 用于创建输出文件
    spdlog::info(" ##1.2. Read anyone image, extract 'resolution', 'coordinate system', and 'data type' as the basic information for outputt data.");
    
    double spacing = 0.;        /// 输出tif的分辨率
    OGRSpatialReference* osr;   /// 输出tif的坐标系统
    GDALDataType datatype;      /// 输出tif的数据类型
    int datasize = 0;           /// 输出tif的数据类型对应的字节类型
    {
        GDALDataset* temp_dataset;
        int i = 0;
        do{
            temp_dataset = (GDALDataset*)GDALOpen(valid_imgpaths[i++].c_str(),GA_ReadOnly);
        }while(!temp_dataset || i >= valid_imgpaths.size());

        if(i >= valid_imgpaths.size())
            return_msg(-1,"there is no image can open by GDAL in valid_imgpaths.");

        GDALRasterBand* rb = temp_dataset->GetRasterBand(1);
        double gt[6];
        temp_dataset->GetGeoTransform(gt);
        spacing = abs(gt[5]);   /// 为了防止高纬度地区经度分辨率大于纬度分辨率的情况, 选用纬度分辨率的绝对值作为输出影像的分辨率
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
    spdlog::info(fmt::format("datatype is {}",GDALGetDataTypeName(datatype)));
    spdlog::info(fmt::format("datasize is {}",datasize));
    spdlog::info(fmt::format("osr.name is {}",osr->GetName()));

#ifdef SYSTEM_PAUSE
    system("pause");
#endif


    /// 2. 从所有有效的影像文件中筛选出与shp有交集（根据相应的筛选策略min or max）的文件，并统计经纬度覆盖范围
    spdlog::info(" #2. Read the shp file, get range of the shp, and extract all the 'geometry' into memory to prevent repeated extraction from increasing time consumption (swapping memory for time) when comparing with DEM one by one in the future.");

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
        spdlog::info(fmt::format("layer.range:left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.",
                envelope_total.MinX, envelope_total.MaxY, envelope_total.MaxX, envelope_total.MinY));
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
            auto g = f->GetGeometryRef()->Buffer(buffer_dist);
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

    spdlog::info("number of geometry in shp is: {}",shp_geometry_vec.size());
    if(shp_geometry_vec.size() < 1){
        return return_msg(-3, "number of geometry in shp < 1");
    }


#ifdef SYSTEM_PAUSE
    system("pause");
#endif

    spdlog::info(" #3. Compare the topological between each DEM's range and the shp file one by one, extract the files that intersect with the shp file, and use them for subsequent concatenation.");

    double contains_lon_max = shp_lon_min, contains_lon_min = shp_lon_max;
    double contains_lat_max = shp_lat_min, contains_lat_min = shp_lat_max;

    int invalid_imgpath_num = 0; 
    int contains_num = 0;
    vector<string> contains_imgpath;
    
    // OGRGeometry* geometry;

    int idx = 0;
    int last_percentage = -1;
#pragma omp parallel for
    for(int i=0; i<valid_imgpaths.size(); i++)
    // for(auto& imgpath : valid_imgpaths)
    {
        int current_percentage = int(idx * 1000 / valid_imgpaths.size());
        if(current_percentage > last_percentage){
            last_percentage = current_percentage;
            std::cout<<fmt::format("\r  preparing percentage {:.1f}%({}/{}): ",last_percentage/10., idx, valid_imgpaths.size());
        }
        idx++;

        string imgpath = valid_imgpaths[i];
        ll_range range = valid_ll_ranges[i];

        double img_lon_min = range.lon_min;
        double img_lon_max = range.lon_max;
        double img_lat_max = range.lat_max;
        double img_lat_min = range.lat_min;

#ifdef PRINT_DETAILS
        std::cout<<fmt::format("img range, left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.",
                    img_lon_min, img_lat_max, img_lon_max, img_lat_min);
#endif
        /// 判断该影像与shp是否相交(广义上的相交，包含计算四至范围(rectangle)和求相交矩形(intersect-rectangle, irregular)两种方法)
        bool b_contains = false;
        if(merging_method == mergingMethod::intersect_rectangle || merging_method == mergingMethod::irregular)
        {
            OGRGeometry* img_geometry = range_to_ogrgeometry(img_lon_min, img_lon_max, img_lat_min, img_lat_max);
            for(int g_idx = 0; g_idx < shp_geometry_vec.size(); g_idx++){
                if(shp_geometry_vec[g_idx]->Intersects(img_geometry)){
                    b_contains = true;
                    break;
                }
            }
            OGRGeometryFactory::destroyGeometry(img_geometry);
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



    spdlog::info(fmt::format("contains range, left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.",
                    contains_lon_min, contains_lat_max, contains_lon_max, contains_lat_min));

    spdlog::info(fmt::format("number of contains_image: {}",contains_num));

#ifdef PRINT_DETAILS
    print_imgpaths("contains_imgpath",contains_imgpath);
#endif

    if(contains_imgpath.size() < 1){
        return return_msg(-6, "contains_imgpath.size() < 1, there is no contained image.");
    }

#ifdef SYSTEM_PAUSE
    system("pause");
#endif

    /// 3. 读取影像文件, 将满足条件的所有影像（contains）写到同一个tif里
    spdlog::info(" #4. Generate output images and assign initial values (short/int: -32767; float: NAN). If the file exists and the size and six parameters are the same, skip the initialization process directly.");

    int width  = (int)ceil((contains_lon_max - contains_lon_min) / spacing);
    int height = (int)ceil((contains_lat_max - contains_lat_min) / spacing);
    double op_gt[6] = {contains_lon_min, spacing, 0, contains_lat_max, 0, -spacing};

    spdlog::info(fmt::format("output_size: width:{}, height:{}",width, height));
    spdlog::info(fmt::format("output_geotranform: {},{},{},{},{},{}",
                    op_gt[0],op_gt[1],op_gt[2],op_gt[3],op_gt[4],op_gt[5]));

    GDALDataset* op_ds;
    GDALRasterBand* op_rb;
    fs::path path_output(op_filepath);
    if(fs::exists(path_output)){
        op_ds = (GDALDataset*)GDALOpen(op_filepath.c_str(),GA_Update);
        if(!op_ds){
            fs::remove(path_output);
            spdlog::warn("output file existed, but open failed, let's remove, create and init it.");
            goto init;
        }
        int temp_width = op_ds->GetRasterXSize();
        int temp_height= op_ds->GetRasterYSize();
        op_rb = op_ds->GetRasterBand(1);
        GDALDataType temp_datatype = op_rb->GetRasterDataType();
        double temp_gt[6];
        op_ds->GetGeoTransform(temp_gt);
        spdlog::info(fmt::format("output file.info, width:{}, height{}, datatype:{}, geotransform: {},{},{},{},{},{}.",
                            temp_width, temp_height,
                            GDALGetDataTypeName(temp_datatype),
                            temp_gt[0],temp_gt[1],temp_gt[2],temp_gt[3],temp_gt[4],temp_gt[5]));

        if(temp_width != width || temp_height != height || temp_datatype != datatype){
            GDALClose(op_ds);
            fs::remove(path_output);
            spdlog::warn(fmt::format("output file is diff with target, let's remove, create and init it."));
            goto init;
        }
        spdlog::info("the existed output file is same with target, so there is no need to init this file.");
    }
    else{
init:
        spdlog::warn(" ##4.1. output file is unexisted, let's create and init it (which will spend a little of time).");
        GDALDriver* driver_tif = GetGDALDriverManager()->GetDriverByName("GTiff");

        char **papszOptions = NULL;
        papszOptions = CSLSetNameValue(papszOptions, "BIGTIFF", "IF_NEEDED");
        op_ds = driver_tif->Create(op_filepath.c_str(), width, height, 1, datatype, papszOptions);
        op_rb = op_ds->GetRasterBand(1);
        op_ds->SetGeoTransform(op_gt);
        if(op_ds->SetSpatialRef(osr) != CE_None){
            spdlog::warn("op_ds->SetSpatialRef(osr) failed.");
        }
        spdlog::info(fmt::format("output_rasterband init...",width, height));
        last_percentage = 0;
        auto init_starttime = chrono::system_clock::now();

        switch (datatype)
        {
        case GDT_Int16:
            {
                short* arr = new short[width];
                for(int i=0; i< width; i++) arr[i] = -32767;
                for(int i = 0; i< height; i++){
                    int current_percentage = i * 1000 / height;
                    if(current_percentage > last_percentage){
                        last_percentage = current_percentage;
                        auto spend = spend_time(init_starttime);
                        size_t remain_sceond = static_cast<size_t>(spend / last_percentage * (1000  - last_percentage));
                        std::cout<<fmt::format("\r  init percentage {:.1f}%({}/{}), remain_time:{}s...            ",last_percentage/10., i, height,remain_sceond);
                    }
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
                    int current_percentage = i * 1000 / height;
                    if(current_percentage > last_percentage){
                        last_percentage = current_percentage;
                        auto spend = spend_time(init_starttime);
                        size_t remain_sceond = static_cast<size_t>(spend / last_percentage * (1000  - last_percentage));
                        std::cout<<fmt::format("\r  init percentage {:.1f}%({}/{}), remain_time:{}s...            ",last_percentage/10., i, height,remain_sceond);
                    }
                    op_rb->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
                }
                delete[] arr;
                op_rb->SetNoDataValue(-32767);
            }
            break;
        case GDT_Float32:
            {
                float* arr = new float[width];
                for(int k=0; k< width; k++) arr[k] = NAN;
                for(int i = 0; i< height; i++){
                    int current_percentage = i * 1000 / height;
                    if(current_percentage > last_percentage){
                        last_percentage = current_percentage;
                        auto spend = spend_time(init_starttime);
                        size_t remain_sceond = static_cast<size_t>(spend / last_percentage * (1000  - last_percentage));
                        std::cout<<fmt::format("\r  init percentage {:.1f}%({}/{}), remain_time:{}s...            ",last_percentage/10., i, height,remain_sceond);
                    }
                    op_rb->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
                }
                delete[] arr;
                op_rb->SetNoDataValue(NAN);
            }
            break;
        }
        std::cout<<"\n";

    }

    /// 3.2 拼接
    spdlog::info(" #5. DEM merging.");
    auto merging_starttime= chrono::system_clock::now();
// #pragma omp parallel for
    for(int i = 0; i< contains_imgpath.size(); i++)
    // for(auto& imgpath : contains_imgpath)
    {
        auto t1= chrono::system_clock::now();

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

        int start_x = (int)round((tmp_gt[0] - op_gt[0]) / op_gt[1]);
        int start_y = (int)round((tmp_gt[3] - op_gt[3]) / op_gt[5]);
#ifdef PRINT_DETAILS
        std::cout<<fmt::format("img in root: start({},{}), size:({},{}), end:({},{})",
                        start_x,start_y,
                        tmp_width, tmp_height,
                        start_x + tmp_width - 1,
                        start_y + tmp_height - 1);
#endif

#if 1
        /// 对于高纬度的地区, 
        int target_width = tmp_width;
        int target_height= tmp_height;

        bool b_height_larger_than_width = false;
        GDALRasterIOExtraArg ex_arg;
        INIT_RASTERIO_EXTRA_ARG(ex_arg);
        if(tmp_height > tmp_width){
            b_height_larger_than_width = true;
            target_width = target_height;
            ex_arg.eResampleAlg = GDALRIOResampleAlg::GRIORA_Bilinear;
        }

        /// 如果使用irrugular方法, 为了不增加太多耗时, 需要实现计算该数据中每个降采样像素快与shp的相交情况
        int step = 100;
        int un_intersect_num = 0;
        int b_target_height= target_height / step;
        int b_target_width = target_width  / step;
        bool* b_arr_intersect = nullptr;
        
        if(merging_method == mergingMethod::irregular)
        {
            b_arr_intersect = new bool[b_target_height * b_target_width];
            /// 如果拼接方式选择irregular, 就需要计算降采样后的像素与shp的相交关系
            for(int row = 0; row < b_target_height; row++){
                for(int col = 0; col < b_target_width; col++){
                    double lon_min = op_gt[0] + (start_x + col * step) * op_gt[1];
                    double lon_max = op_gt[0] + (start_x + (col+1) * step) * op_gt[1];
                    double lat_max = op_gt[3] + (start_y + row * step) * op_gt[5];
                    double lat_min = op_gt[3] + (start_y + (row+1) * step) * op_gt[5];
                    bool b = false;
                    OGRGeometry* img_geometry = range_to_ogrgeometry(lon_min, lon_max, lat_min, lat_max);
                    for(auto& geometry : shp_geometry_vec){
                        b = b || geometry->Intersects(img_geometry);
                    }
                    OGRGeometryFactory::destroyGeometry(img_geometry);

                    b_arr_intersect[row * b_target_width + col] = b;
                    if(!b) un_intersect_num++;
                }
            }
        }

        // void* arr = malloc(tmp_width * tmp_height * datasize);
        void* arr = malloc(target_width * target_height * datasize);

        
        rb->RasterIO(GF_Read, 0, 0, tmp_width, tmp_height, arr, target_width, target_height, datatype, 0, 0, b_height_larger_than_width ? &ex_arg : NULL);
        // rb->RasterIO(GF_Read, 0, 0, tmp_width, tmp_height, arr, tmp_width, tmp_height, datatype, 0, 0);

        if(merging_method == mergingMethod::irregular)
        {
            switch (datatype)
            {
            case GDT_Int16:{
                short* p_arr = (short*)arr;
                for(int row = 0; row < target_height; row++){
                    for(int col = 0; col < target_width; col++){
                        int b_arr_index = row / step * b_target_width + col/step;
                        if(!b_arr_intersect[b_arr_index]){
                            p_arr[row * target_width + col] = -32767;
                        }
                    }
                }
                }break;
            case GDT_Int32:{
                int* p_arr = (int*)arr;
                for(int row = 0; row < target_height; row++){
                    for(int col = 0; col < target_width; col++){
                        int b_arr_index = row / step * b_target_width + col/step;
                        if(!b_arr_intersect[b_arr_index]){
                            p_arr[row * target_width + col] = -32767;
                        }
                    }
                }
                }break;
            case GDT_Float32:{
                float* p_arr = (float*)arr;
                for(int row = 0; row < target_height; row++){
                    for(int col = 0; col < target_width; col++){
                        int b_arr_index = row / step * b_target_width + col/step;
                        if(!b_arr_intersect[b_arr_index]){
                            p_arr[row * target_width + col] = NAN;
                        }
                    }
                }
                }break;
            }
            delete[] b_arr_intersect;
        }

        op_rb->RasterIO(GF_Write, start_x, start_y, target_width, target_height, arr, target_width, target_height, datatype, 0, 0);
        
#else
        void* arr = malloc(tmp_width * datasize);

        for(int ii = 0; ii < tmp_height; ii++)
        {
            rb->RasterIO(GF_Read, 0, ii, tmp_width, 1, arr, tmp_width, 1, datatype, 0, 0);
            op_rb->RasterIO(GF_Write, start_x, start_y + ii, tmp_width, 1, arr, tmp_width, 1, datatype, 0, 0);
        }

#endif
        free(arr);
        GDALClose(ds);


        auto t2= chrono::system_clock::now();
        if( i != 0){
            auto spend = spend_time(merging_starttime);
            size_t remain_sceond = size_t(spend / i * (contains_num  - i));
            std::cout<<fmt::format("\r  merging percentage {:.1f}%({}/{}), time of parent rasterio spend:{}s, remain_time:{}s...            ",
            i*100./contains_num, i, contains_num,
            spend_time(t1,t2), 
            remain_sceond);
        }
    }
    std::cout<<"\n";

    GDALClose(op_ds);
    
    destrory_geometrys();
    
    
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