#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"

#define EXE_NAME "read_EGM2008"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum class method{unknown, point, txt, dem};

bool write_all_egm2008(const char* in_path, const char* out_path);

int main(int argc, char* argv[])
{

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

   
    if(argc < 2){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [egm_filepath] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, there are three type, point(lon,lat), points_filepath(multi points), dem filepath.\n"
                " argv[2]: egm_filepath, only support '*_SE' egm file."
                " argv[3]: output, which dependent by input string, point(none, output on terminal), points(multi points), err_hei filepath.\n";
        return return_msg(-1,msg);
    }

    /// 仅测试时使用, 打印所有的数字, 以逗号为分隔符
    if(string(argv[1]) == "output_all"){
        if(write_all_egm2008(argv[2], argv[3])){
            cout<<"write all egm2008 success."<<endl;
        }else{
            cout<<"write all egm2008 failed."<<endl;
        }
        return 0;
    }

    return_msg(0,EXE_NAME " start.");

    /// step.1 判断是单点计算还是批量还是DEM
    method input_method = method::unknown; 

    double lon, lat;

    std::string str_input = argv[1];
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
        /// 确定输入的是文件后, 就需要保证输出项也填了, 否则报错
        if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [egm_filepath] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, there are three type, point(lon,lat), point_filepath(multi points), dem filepath.\n"
                " argv[2]: egm_filepath, only support '*_SE' egm file."
                " argv[3]: output, which dependent by input string, point(lon,lat,err_hei), points(multi points), err_hei filepath.\n";
        return return_msg(-2,msg);
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
    funcrst rst = egm.init(argv[2]);
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
        rst = egm.write_height_anomaly_txt(argv[1], argv[3]);
        return return_msg(-3,rst.explain);
        }
        break;
    case method::dem:{
        GDALAllRegister();
        GDALDataset* ds_in  = (GDALDataset*)GDALOpen(argv[1],GA_ReadOnly);
        if(!ds_in){
            return return_msg(-3,"ds_in is nullptr");
        }
        GDALRasterBand* rb_in = ds_in->GetRasterBand(1);
        int dem_width = ds_in->GetRasterXSize();
        int dem_height = ds_in->GetRasterYSize();
        double dem_gt[6];
        ds_in->GetGeoTransform(dem_gt);

        GDALDriver* tif_driver = GetGDALDriverManager()->GetDriverByName("GTiff");
        GDALDataset* ds_out = tif_driver->Create(argv[3],dem_width, dem_height,1,GDT_Float32,nullptr);
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


    return return_msg(1, EXE_NAME " end.");
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
}