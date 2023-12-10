#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <omp.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>
#include <fmt/format.h>

#include "datatype.h"

#define EXE_NAME "image_cut_pixel"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}


int main(int argc, char* argv[])
{

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        if(rtn >= 0){
            my_logger->info(msg);
            spdlog::info(msg);
        }
        else{
            my_logger->error(msg);
            spdlog::error(msg);
        }
        
        return rtn;
    };

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [img in] [parameters] [img out]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, image filepath .\n"
                " argv[2]: input, 4 int parameters splited by ',': start_x, start_y, width, height .\n"
                " argv[3]: output, cutted image filepath.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    vector<string> splited_strings;
    strSplit(string(argv[2]), splited_strings, ",");
    if(splited_strings.size() < 4){
        return return_msg(-2,fmt::format("the size of splited argv[2] is {} < 4.",splited_strings.size()));
    }

    int start_x = stoi(splited_strings[0]);
    int start_y = stoi(splited_strings[1]);
    int cutted_width  = stoi(splited_strings[2]);
    int cutted_height = stoi(splited_strings[3]);

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly);
    if(!ds){
        return return_msg(-2, "ds is nullptr.");
    }
    GDALRasterBand* rb = ds->GetRasterBand(1);

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    GDALDataType datatype = rb->GetRasterDataType();
    int datasize = GDALGetDataTypeSize(datatype);
    double geotransform[6];
    auto cplerr = ds->GetGeoTransform(geotransform);
    bool has_geotransform = (cplerr == CE_Failure ? false : true);

    /// 调整裁剪范围
    if(start_x < 0) start_x = 0;
    if(start_y < 0) start_y = 0;
    if(start_x + cutted_width > width) cutted_width = width - start_x;
    if(start_y + cutted_height > height) cutted_height = height - start_y;


    GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(argv[3], cutted_width, cutted_height, 1, datatype, NULL);
    if(!ds_out){
        return return_msg(-3, "ds_out create failed.");
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
    if(has_geotransform){
        double geotransform_out[6];
        geotransform_out[0] = geotransform[0] + start_x * geotransform[1] + start_y * geotransform[2];
        geotransform_out[1] = geotransform[1];
        geotransform_out[2] = geotransform[2];
        geotransform_out[3] = geotransform[3] + start_x * geotransform[4] + start_y * geotransform[5];
        geotransform_out[4] = geotransform[4];
        geotransform_out[5] = geotransform[5];
        ds_out->SetGeoTransform(geotransform_out);
        ds_out->SetProjection(ds->GetProjectionRef());
    }


    void* arr = malloc(cutted_width * datasize);
    for(int i=0; i< cutted_width; i++)
    {
        rb->RasterIO(GF_Read, start_x, i + start_y, cutted_width, 1, arr, cutted_width, 1, datatype, 0, 0);
        rb_out->RasterIO(GF_Write, 0, i, cutted_width, 1, arr, cutted_width, 1, datatype, 0, 0);
    }
    free(arr);

    GDALClose(ds);
    GDALClose(ds_out);

    return return_msg(1, EXE_NAME " end.");
}
