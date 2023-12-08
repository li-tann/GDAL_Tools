#include <gdal_priv.h>

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "template_binary_img_io.h"

#define EXE_NAME "binary_to_tif"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

#define DATATYPE_LENGTH 4
enum binary_datatype{binary_unknown, binary_int, binary_float, binary_fcomplex};
std::string str_datatype[DATATYPE_LENGTH] = {"unknown", "int", "float", "fcomplex"};

using namespace std;

int main(int argc, char* argv[])
{
    GDALAllRegister();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    if(argc < 7){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input binary image filepath,\n"
                " argv[2]: img width,\n"
                " argv[3]: img height,\n"
                " argv[4]: img datatype, support list: int, float, fcomplex ...\n"
                " argv[5]: high-low byte trans, true of false,\n"
                " argv[6]: output tif image filepath,\n";
        return return_msg(-1,msg);
    }

    msg = string(EXE_NAME) + " start.";
    return_msg(0,msg);


    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    binary_datatype bin_datatype =binary_unknown;
    for(int i=0; i< DATATYPE_LENGTH; i++){
        if(str_datatype[i] ==  argv[4])
            bin_datatype = binary_datatype(i);
    }

    if(bin_datatype == binary_unknown){
        return return_msg(-2,"unsupported datatype.");
    }

    bool high_low_byte_trans = true;
    if(argv[5] == "false"){
        high_low_byte_trans = false;
    }
     
    switch (bin_datatype)
    {
    case binary_int:{
        binary_tif_io<int> btio(width,height);
        auto ans = btio.read_binary(argv[1], high_low_byte_trans);
        if(!std::get<0>(ans))
            return return_msg(-4, std::get<1>(ans));
        ans = btio.write_tif(argv[6],GDT_Int32);
        if(!std::get<0>(ans))
            return return_msg(-5, std::get<1>(ans));
    }break;
    case binary_float:{
        binary_tif_io<float> btio(width,height);
        auto ans = btio.read_binary(argv[1], high_low_byte_trans);
        if(!std::get<0>(ans))
            return return_msg(-4, std::get<1>(ans));
        ans = btio.write_tif(argv[6],GDT_Float32);
        if(!std::get<0>(ans))
            return return_msg(-5, std::get<1>(ans));
    }break;
    case binary_fcomplex:{
        binary_tif_io_cpx<float> btio(width,height);
        auto ans = btio.read_binary_cpx(argv[1], high_low_byte_trans);
        if(!std::get<0>(ans))
            return return_msg(-4, std::get<1>(ans));
        ans = btio.write_tif_cpx(argv[6],GDT_CFloat32);
        if(!std::get<0>(ans))
            return return_msg(-5, std::get<1>(ans));
    }break;
    default:
        return return_msg(-3, "unknown datatype.");
        break;
    }


    msg = string(EXE_NAME) + " successed.";
    return return_msg(1, msg);

}


