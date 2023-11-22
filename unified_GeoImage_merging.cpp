#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <regex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include "datatype.h"

#define EXE_NAME "_unified_geoimage_merging"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

/// TODO: 需要解决的问题
///         1. 由dem生成的简单shp能否与目标shp计算是否存在交集(应该可以, 参考LTIE的dem拼接代码(那里有算两个面矢量的相交比例))
///         2. 由目标shp是否能得到shp对应的经纬度范围
///         3. 拼接策略, a)是只拼接与目标shp有交集的DEM，但遇到不规则的shp时, 拼接结果的边边角角可能会有很多空值
///                     b)是将shp经纬度范围内的所有DEM都拼上，这样虽然不会有空值，但也失去了shp的形状特点

/// 输入经纬度范围, 转换为OGRGeometry格式, 后面可计算交集
OGRGeometry* range_to_ogrgeometry(double lon_min, double lon_max, double lat_min, double lat_max);

enum class mergingMethod{minimum, maximum};

/// 该测试案例可成功通过 可用".*DEM.tif"来搜索后缀是DEM.tif的文件
int regex_test(); 

void print_imgpaths(string vec_name, vector<string>& paths);

int main(int argc, char* argv[])
{
    // return regex_test();

    argc = 6;
    argv = new char*[6];
    for(int i=0; i<6; i++){
        argv[i] = new char[256];
    }
    strcpy(argv[1], "D:\\1_Data\\shp_test\\TanDEM_DEM");
    strcpy(argv[2], "D:\\1_Data\\shp_test\\poly_1.shp");
    strcpy(argv[3], "0");
    strcpy(argv[4], ".*DEM.tif");
    strcpy(argv[5], "D:\\1_Data\\shp_test\\poly_1.dem.tif");

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

    /// 1.读取影像文件夹内所有影像信息待用, 并从中任选一个数据提取分辨率、坐标系统和数据类型信息。

    fs::path root_path(argv[1]);
    if(!fs::exists(root_path)){
        return return_msg(-4,"argv[1] is not existed.");
    }

    vector<string> valid_imgpaths;
    regex reg_f(regex_regular);
    smatch result;
    int file_num = 0, valid_file_num = 0;

    /// 1.1.迭代查看当前路径及子文件内的所有文件/文件夹， 筛选符合条件的文件作为参与拼接的图像数据
    for (auto& iter : fs::recursive_directory_iterator(root_path)){
        if(iter.is_directory())
            continue;
        ++file_num;

        string filename = iter.path().filename().string();
        bool res = regex_match(filename, result, reg_f);
        if(b_regex && !res)
            continue;
        ++valid_file_num;

        valid_imgpaths.push_back(iter.path().string());
    }
    std::cout<<fmt::format("valid_file_percentage : {}/{}({:.2f}%)\n",valid_file_num, file_num, 100.*valid_file_num/file_num);
    if(valid_file_num == 0){
        return return_msg(-5, "there is no valid file in argv[1].");
    }
    print_imgpaths("valid_imgpaths",valid_imgpaths);

    // system("pause");

    /// 1.2. 任选其一获取基本信息（分辨率、坐标系统、数据类型）, 用于创建输出文件
    double spacing = 0.;
    OGRSpatialReference* osr;
	
    GDALDataType datatype;
    int datasize = 0;
    {
        GDALDataset* temp_dataset;
        int i = 0;
        do{
            temp_dataset = (GDALDataset*)GDALOpen(valid_imgpaths[i++].c_str(),GA_ReadOnly);
        }while(!temp_dataset || i < valid_imgpaths.size());
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

    // system("pause");

    /// 2. 从所有有效的影像文件中筛选出与shp有交集（根据相应的筛选策略min or max）的文件，并统计经纬度覆盖范围

    GDALDataset* shp_dataset = (GDALDataset*)GDALOpenEx(argv[2], GDAL_OF_VECTOR, NULL, NULL, NULL);
    if(!shp_dataset){
        return return_msg(-2,"invalid argv[2], open shp_dataset failed.");
    }
    OGRLayer* layer = shp_dataset->GetLayer(0);
    layer->ResetReading();

    OGRFeature* feature;
    // int feature_num = 0;
    // while ((feature = layer->GetNextFeature()) != NULL){
    //     ++feature_num;
    // }
    // layer->ResetReading();
    // std::cout<<fmt::format("number of feature is {}\n",feature_num);

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

    int invalid_imgpath_num = 0, contains_img_num = 0; 
    double contains_lon_max = shp_lon_min, contains_lon_min = shp_lon_max;
    double contains_lat_max = shp_lat_min, contains_lat_min = shp_lat_max;
    vector<string> contains_imgpath;
    for(auto& imgpath : valid_imgpaths)
    {
        GDALDataset* ds = (GDALDataset*)GDALOpen(imgpath.c_str(), GA_ReadOnly);
        if(!ds){
            invalid_imgpath_num++;
            continue;
        }

        double gt[6];
        ds->GetGeoTransform(gt);
        int width = ds->GetRasterXSize();
        int height= ds->GetRasterYSize();

        GDALClose(ds);

        double img_lon_min = gt[0], img_lon_max = gt[0] +  width * gt[1];
        double img_lat_max = gt[3], img_lat_min = gt[3] + height * gt[5];

        std::cout<<fmt::format("img range, left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.\n",
                    img_lon_min, img_lat_max, img_lon_max, img_lat_min);
        
        /// 判断该影像与shp是否相交(广义上的相交，包含minimum和maximum两种相交方法)
        bool b_contains = false;
        if(merging_method == mergingMethod::minimum)
        {
            OGRGeometry* img_geometry = range_to_ogrgeometry(img_lon_min, img_lon_max, img_lat_min, img_lat_max);
            layer->ResetReading();
            while ((feature = layer->GetNextFeature()) != NULL){
                OGRGeometry* geometry = feature->GetGeometryRef();
                if (geometry != NULL){
                    // if (geometry->Contains(img_geometry)){
                    //     b_contains = true;
                    //     break;
                    // }
                    if (geometry->Overlaps(img_geometry)){
                        b_contains = true;
                        break;
                    }
                }
                img_geometry->closeRings();
            }
            OGRFeature::DestroyFeature(feature);
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
        ++contains_img_num;

        contains_lon_max = MAX(img_lon_max, contains_lon_max);
        contains_lon_min = MIN(img_lon_min, contains_lon_min);
        contains_lat_max = MAX(img_lat_max, contains_lat_max);
        contains_lat_min = MIN(img_lat_min, contains_lat_min);
        contains_imgpath.push_back(imgpath);
    }
    GDALClose(shp_dataset);

    std::cout<<fmt::format("contains range, left:{:.4f}, top:{:.4f}, right:{:.4f}, down:{:.4f}.\n",
                    contains_lon_min, contains_lat_max, contains_lon_max, contains_lat_min);

    std::cout<<"contains_imgpath.size:"<<contains_imgpath.size()<<endl;
    print_imgpaths("contains_imgpath",contains_imgpath);
    // system("pause");

    /// 3. 读取影像文件, 根据策略进行比较, 生成到

    int width  = ceil((contains_lon_max - contains_lon_min) / spacing);
    int height = ceil((contains_lat_max - contains_lat_min) / spacing);
    double op_gt[6] = {contains_lon_min, spacing, 0, contains_lat_max, 0, -spacing};

    std::cout<<fmt::format("output_size: width:{}, height:{}\n",width, height);
    std::cout<<fmt::format("output_geotranform: {:.6f},{:.6f},{:.6f},{:.6f},{:.6f},{:.6f}\n",
                    op_gt[0],op_gt[1],op_gt[2],op_gt[3],op_gt[4],op_gt[5]);

    GDALDriver* driver_tif = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* op_dataset = driver_tif->Create(op_filepath.c_str(), width, height, 1, datatype, NULL);
    if(!op_dataset){
        return return_msg(-6, "output filepath is invalid.");
    }
    GDALRasterBand* op_rb = op_dataset->GetRasterBand(1);
    op_dataset->SetGeoTransform(op_gt);
    if(op_dataset->SetSpatialRef(osr) != CE_None){
        cout<<"op_dataset->SetSpatialRef(osr) failed."<<endl;
    }

    /// 赋初始值
    switch (datatype)
    {
    case GDT_Int16:
        {
            short* arr = new short[width];
            for(int i=0; i< width; i++) arr[i] = -32767;
            for(int i = 0; i< height; i++){
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
                op_rb->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            }
            delete[] arr;
            op_rb->SetNoDataValue(NAN);
        }
        break;
    }

    /// 拼接
    for(auto& imgpath : contains_imgpath)
    {
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

        std::cout<<fmt::format("img in root: start({},{}), size:({},{}), end:({},{})\n\tpath:[{}]\n",
                        start_x,start_y,
                        tmp_width, tmp_height,
                        start_x + tmp_width - 1,
                        start_y + tmp_height - 1,
                        imgpath);

        void* arr = malloc(tmp_width * tmp_height * datasize);
        
        rb->RasterIO(GF_Read, 0, 0, tmp_width, tmp_height, arr, tmp_width, tmp_height, datatype, 0, 0);
        op_rb->RasterIO(GF_Write, start_x, start_y, tmp_width, tmp_height, arr, tmp_width, tmp_height, datatype, 0, 0);
        
        free(arr);
        GDALClose(ds);
    }

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