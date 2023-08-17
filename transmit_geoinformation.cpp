#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


#define EXE_NAME "_transmit_geoinformation"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

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

    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: reference imgpath, which has geoinformation,\n"
                " argv[2]: target imgpath, which the reference geoinformation transmit to.";
        return return_msg(-1,msg);
    }

    msg = string(EXE_NAME) + " start.";
    return_msg(0,msg);

    GDALDataset* ds_in = static_cast<GDALDataset*>(GDALOpen(argv[1],GA_ReadOnly));
    if(ds_in == nullptr)
        return return_msg(-2,"ds_in is nullptr.");

    GDALDataset* ds_out = static_cast<GDALDataset*>(GDALOpen(argv[2],GA_Update));
    if(ds_out == nullptr)
        return return_msg(-3,"ds_out is nullptr.");
    
    double gt[6];
    ds_in->GetGeoTransform(gt);
    ds_out->SetGeoTransform(gt);

    ds_out->SetProjection(ds_in->GetProjectionRef());
    
    GDALClose(ds_in);
    GDALClose(ds_out);
    msg = string(EXE_NAME) + " successed.";
    return return_msg(1, msg);

}


    