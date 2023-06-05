#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"

#define EXE_NAME "_read_EGM2008"

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

    std::string str_input = argv[1];
    fs::path input_path(str_input);
    if(fs::exists(input_path)){
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

    /// 根据method输出数据


    /// 剩余步骤: 输入经纬度, 计算对应的行列号, 插值


    switch (input_method)
    {
    case method::point:{
        /* code */
        }break;
    case method::txt:{
        /* code */
        }break;
    case method::dem:{
        /* code */
        }break;
    }

    



    


    /// 建立一个EGM的类, 具备功能: 给定




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