#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"

using namespace std;
namespace fs = std::filesystem;

enum class method{unknown, point, txt, dem};

bool write_all_egm2008(const char* in_path, const char* out_path);

int main(int argc, char* argv[])
{

    argparse::ArgumentParser program("create_delaunay", "1.0");
    program.add_description("input points, construct delaunay network and output");

    program.add_argument("input")
        .help("input, choose one of the three modes for input, 1:point(lon,lat), 2:points_filepath(multi points, *.txt), 3:dem filepath(*.tif).");

    program.add_argument("egm")
        .help("input *_SE EGM filepath");

    program.add_argument("-o","--output")
        .help("output mode dependent by input mode, point(output on terminal), points_file(multi points, *.txt), err_hei filepath(*.tif).");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl<<std::endl;
        std::cerr << program;
        return 1;
    }

    /// log
    char* pgmptr = 0;
    _get_pgmptr(&pgmptr);
    fs::path exe_root(fs::path(pgmptr).parent_path());
    fs::path log_path = exe_root / "read_egm2008.log";
    auto my_logger = spdlog::basic_logger_mt("read_egm2008", log_path.string());

    auto start = chrono::system_clock::now();
    string msg;

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    return_msg(0,"read_egm2008 start.");

    /// step.1 判断是单点计算还是批量还是DEM
    method input_method = method::unknown; 

    double lon, lat;

    string str_input = program.get<string>("input");
    string egm_filepath = program.get<string>("egm");
    string str_output = program.get<string>("output");

    fs::path input_path(str_input);

    if(fs::exists(input_path)){
        /// 如果确定输入的是一个文件, 那么以文件对待, 进行判断
        if(input_path.extension() ==".tif" || input_path.extension() ==".tiff" /* || input_path.extension() == ".hgt" */){
            input_method = method::dem;
        }else if(input_path.extension() ==".txt"){
            input_method = method::txt;
        }
        else{
            msg = "input is not a standard file, which extension is exclude with tif, tiff or txt.";
            return return_msg(-2,msg);
        }
        if(program.is_used("--output")){
            return_msg(-1,"when inputting a file, '--output' is a mandatory option");
            std::cerr << program;
            return 1;
        }
    }
    else{
        vector<string> splited;
        strSplit(str_input, splited,",");
        if(splited.size()<2){
            msg = "input is not a file, else not a stanard point type(lon, lat).";
            return return_msg(-2,msg);
        }
        input_method = method::point;
        lon = stod(splited[0]);
        lat = stod(splited[1]);

        if(lon > 180 || lon < -180 || lat > 90 || lat < -90){
             msg = "the coordinates input are not valid value, please ensure that the longitude and latitude are within the range[-180,180], [-90, 90] ";
            return return_msg(-2,msg);
        }
    }

    /// 识别EGM类型, 不能只靠文件名

    egm2008 egm;
    funcrst rst = egm.init(egm_filepath.c_str());
    if(!rst){
        return return_msg(-3,rst.explain);
    }

    cout<<"height: "<<egm.height<<endl;
    cout<<"width:  "<<egm.width<<endl;
    cout<<"spacing:"<<egm.spacing<<endl;

    // cout<<"arr[n*1080]"<<egm.arr[1080]<<","<<egm.arr[2*1080]<<","<<egm.arr[3*1080]<<","<<egm.arr[4*1080]<<","<<egm.arr[5*1080]<<endl;

    /// 根据method输出数据


    /// 剩余步骤: 输入经纬度, 计算对应的行列号, 插值


    switch (input_method)
    {
    case method::point:{
        float height_anomaly = egm.calcluate_height_anomaly_single_point(lon, lat);
        cout<<fmt::format("cal single_point's height anomaly: {}.",height_anomaly)<<endl;
        }
        break;
    case method::txt:{
        rst = egm.write_height_anomaly_txt(str_input.c_str(), str_output.c_str());
        return return_msg(-3,rst.explain);
        }
        break;
    case method::dem:{
        GDALAllRegister();
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

        GDALDataset* ds_in  = (GDALDataset*)GDALOpen(str_input.c_str(), GA_ReadOnly);
        if(!ds_in){
            return return_msg(-3,"ds_in is nullptr");
        }
        GDALRasterBand* rb_in = ds_in->GetRasterBand(1);
        int dem_width = ds_in->GetRasterXSize();
        int dem_height = ds_in->GetRasterYSize();
        double dem_gt[6];
        ds_in->GetGeoTransform(dem_gt);

        GDALDriver* tif_driver = GetGDALDriverManager()->GetDriverByName("GTiff");
        GDALDataset* ds_out = tif_driver->Create(str_output.c_str(), dem_width, dem_height,1,GDT_Float32,nullptr);
        if(!ds_out){
            return return_msg(-3,"ds_out is nullptr");
        }
        float* interped_arr = new float[dem_height * dem_width];
        rst = egm.write_height_anomaly_image(dem_height, dem_width, dem_gt, interped_arr);
        return_msg(-4,rst.explain);
        if(!rst){
            return return_msg(-4,rst.explain);
        }

        cout<<"interped_arr.size():"<<dynamic_array_size(interped_arr)<<endl;

        GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
        rb_out->RasterIO(GF_Write,0,0,dem_width, dem_height, interped_arr,dem_width, dem_height,GDT_Float32,0,0);

        ds_out->SetGeoTransform(dem_gt);
        ds_out->SetProjection(ds_in->GetProjectionRef());
        

        delete[] interped_arr;
        GDALClose(ds_in);
        GDALClose(ds_out);
        }
        break;
    }


    return return_msg(1, "read_egm2008 end.");
}

bool write_all_egm2008(const char* in_path, const char* out_path)
{
    ifstream ifs(in_path, ifstream::binary); //二进制读方式打开
    if (!ifs.is_open()) {
        printf("ERROR: File open failed...\nFilepath is %s",string(in_path));
        return false;
    }

    ofstream ofs(out_path);
    if(!ofs.is_open()){
         printf("ERROR: File write failed...\nFilepath is %s",string(out_path));
        return false;
    }

    long num = 0;
    float value;
    while (ifs.read((char*)&value, 4)) { //一直读到文件结束
        ofs<<value<<",";
        ++num;
    }
    ofs<<"\nnum:"<<num;

    ifs.close();
    ofs.close();

    return true;
}