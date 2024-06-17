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

#define EXE_NAME "tif_to_binary"

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

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input tif filepath, driver is gtiff,\n"
                " argv[2]: high-low byte trans, true of false,\n"
                " argv[3]: output binary image filepath,\n";
        return return_msg(-1,msg);
    }

    msg = string(EXE_NAME) + " start.";
    return_msg(0,msg);

    bool high_low_byte_trans = true;
    if(argv[2] == "false"){
        high_low_byte_trans = false;
    }

    GDALAllRegister();
    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(argv[1],GA_ReadOnly));
    if(ds == nullptr){
        return return_msg(-2,"ds is nullptr.");
    }
    int width = ds->GetRasterXSize();
    int height = ds->GetRasterYSize();
    /// 只提取band1
    GDALDataType datatype = ds->GetRasterBand(1)->GetRasterDataType();
    GDALClose(ds);

    switch (datatype)
    {
    case GDT_Float32:{
        binary_tif_io<float> btio(width, height);
        auto ans = btio.read_tif(argv[1]);
        if(!std::get<0>(ans))
            return return_msg(-3, std::get<1>(ans));
        ans = btio.write_binary(argv[3]);
        if(!std::get<0>(ans))
            return return_msg(-4, std::get<1>(ans));
    }break;
    case GDT_CFloat32:{
        binary_tif_io_cpx<float> btio(width,height);
        auto ans = btio.read_tif_cpx(argv[1]);
        if(!std::get<0>(ans))
            return return_msg(-3, std::get<1>(ans));
        ans = btio.write_binary_cpx(argv[3]);
        if(!std::get<0>(ans))
            return return_msg(-4, std::get<1>(ans));
    }break;
    default:
        return return_msg(-5, "unknown datatype.");
        break;
    }

    msg = string(EXE_NAME) + " successed.";
    return return_msg(1, msg);

}


