#include <gdal_priv.h>

#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "template_nan_convert_to.h"

#define EXE_NAME "_binary_to_tif"

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

    if(argc < 5){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input binary image filepath, datatype is float,\n"
                " argv[2]: img width,\n"
                " argv[3]: img height,\n"
                " argv[4]: output tif image filepath,\n";
        return return_msg(-1,msg);
    }
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);

    msg = string(EXE_NAME) + " start.";
    return_msg(0,msg);

    ifstream ifs(argv[1],ifstream::binary);
    if(!ifs.is_open()){
        return return_msg(-2,"ifstream::open argv[1] failed.");
    }



    

    float* arr = new float[width * height];
    int num = 0;
    unsigned int value2;
    while (ifs.read((char*)&value2, 4)) { //一直读到文件结束
        //高低位字节变换
        unsigned int idata1, idata2, idata3, idata4;
        idata1 = 255 & (value2 >> 24);
        idata2 = 255 & (value2 >> 16);
        idata3 = 255 & (value2 >> 8);
        idata4 = 255 & value2;
        char str[9];
        sprintf(str, "%02x%02x%02x%02x", idata4, idata3, idata2, idata1);
        str[8] = 0;
        float val;
        sscanf(str, "%x", &val);
        
        arr[num++] = val;
    }

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds = driver->Create(argv[4],width,height,1,GDT_Float32,nullptr);

    if(ds == nullptr)
        return return_msg(-2,"ds is nullptr.");

    CPLErr err = ds->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, width, height, arr, width, height, GDT_Float32, 0, 0);
    if(err == CE_Failure){
        return return_msg(-3,"ds.rasterio.write failed.");
    }

    GDALClose(ds);

     msg = string(EXE_NAME) + " successed.";
    return return_msg(1, msg);

}


