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

#define EXE_NAME "_binary_to_tif"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum binary_datatype{binary_unknown, binary_int, binary_float};
std::string str_datatype[3] = {"unknown", "int", "float"};

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

    if(argc < 6){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input binary image filepath, datatype is float,\n"
                " argv[2]: img width,\n"
                " argv[3]: img height,\n"
                " argv[4]: img datatype, support list: int, float, ...\n"
                " argv[5]: output tif image filepath,\n";
        return return_msg(-1,msg);
    }

    msg = string(EXE_NAME) + " start.";
    return_msg(0,msg);


    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    binary_datatype bin_datatype =binary_unknown;
    for(int i=0; i< 3; i++){
        if(str_datatype[i] ==  argv[4])
            bin_datatype = binary_datatype(i);
    }

    if(bin_datatype == binary_unknown){
        return return_msg(-2,"unsupported datatype.");
    }
     
    switch (bin_datatype)
    {
    case binary_int:{
        binary_tif_io<int>* btio = new binary_tif_io<int>(width,height);
        btio->read_binary(argv[1]);
        btio->write_tif(argv[5],GDT_Int32);
        delete[] btio;
        }break;

    case binary_float:{
        // binary_tif_io<float>* btio = new binary_tif_io<float>(width,height);
        binary_tif_io<float> btio(width,height);
        auto ans = btio.read_binary(argv[1]);
        if(!std::get<0>(ans)){
            return return_msg(-4, std::get<1>(ans));
        }
        ans = btio.write_tif(argv[5],GDT_Float32);
        if(!std::get<0>(ans)){
            return return_msg(-5, std::get<1>(ans));
        }
        }break;

    default:
        return return_msg(-3, "unknown datatype.");
        break;
    }


    msg = string(EXE_NAME) + " successed.";
    return return_msg(1, msg);

}


