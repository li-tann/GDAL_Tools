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

#define EXE_NAME "complex_transto"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum class _method{real, imag, power, amplitude, phase};

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
        msg +=  " manual: " EXE_NAME " [cpx filepath] [method] [real filepath]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, complex filepath, scomplex or fcomplex.\n"
                " argv[2]: input, method, real_part, imag_part, power, amplitude, phase.\n"
                " argv[3]: output, real filepath.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    _method method;
    if(string(argv[2]) == "real_part")
        method = _method::real;
    else if(string(argv[2]) == "imag_part")
        method = _method::imag;
    else if(string(argv[2]) == "power")
        method = _method::power;
    else if(string(argv[2]) == "amplitude")
        method = _method::amplitude;
    else if(string(argv[2]) == "phase")
        method = _method::phase;
    else
        return_msg(-1, fmt::format("unknown method({}).",argv[2]));

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

    if(datatype != GDT_CFloat32){
        return return_msg(-3, "datatype is not fcpx.");
    }

    GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(argv[3], width, height, 1, GDT_Float32, NULL);
    if(!ds_out){
        return return_msg(-3, "ds_out create failed.");
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);

    switch (method)
    {
    case _method::real:{
        float* arr = new float[2*width];
        float* arr2 = new float[width];
        for(int i=0; i<height; i++){
            rb->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int i=0; i< width; i++)
                arr2[i] = arr[2*i];
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, GDT_Float32, 0, 0);
        }
        delete[] arr;
        delete[] arr2;
        }break;
    case _method::imag:{
        float* arr = new float[2*width];
        float* arr2 = new float[width];
        for(int i=0; i<height; i++){
            rb->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int i=0; i< width; i++)
                arr2[i] = arr[2*i+1];
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr2, width, 1, GDT_Float32, 0, 0);
        }
        delete[] arr;
        delete[] arr2;
        }break;
    case _method::power:{
        float* arr = new float[2*width];
        float* arr2 = new float[width];
        for(int i=0; i<height; i++){
            rb->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int i=0; i< width; i++)
                arr2[i] = powf(arr[2*i],2) + powf(arr[2*i+1],2);
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr2, width, 1, GDT_Float32, 0, 0);
        }
        delete[] arr;
        delete[] arr2;
        }break;
    case _method::amplitude:{
        float* arr = new float[2*width];
        float* arr2 = new float[width];
        for(int i=0; i<height; i++){
            rb->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int i=0; i< width; i++)
                arr2[i] = sqrtf(powf(arr[2*i],2) + powf(arr[2*i+1],2));
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr2, width, 1, GDT_Float32, 0, 0);
        }
        delete[] arr;
        delete[] arr2;
        }break;
    case _method::phase:{
        float* arr = new float[2*width];
        float* arr2 = new float[width];
        for(int i=0; i<height; i++){
            rb->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int i=0; i< width; i++)
                arr2[i] = atan2f(arr[2*i+1], arr[2*i]);
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr2, width, 1, GDT_Float32, 0, 0);
        }
        delete[] arr;
        delete[] arr2;
        }break;
    }

    GDALClose(ds);
    GDALClose(ds_out);
    
    return return_msg(1, EXE_NAME " end.");
}
