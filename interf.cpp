#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <omp.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/format.h>

#include <gdal_priv.h>

#include <fftw3.h>

#include "datatype.h"

#define EXE_NAME "interf"

// #define DEBUG

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
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [params] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]:  input, master rslc filepath.\n"
                " argv[2]:  input,  slave rslc filepath.\n"
                " argv[3]:  input, int filepath.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");
    
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_mas = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly);
    if(!ds_mas){
        return funcrst(false, "ds_mas is nullptr");
    }
    GDALRasterBand* rb_mas = ds_mas->GetRasterBand(1);

    int height = ds_mas->GetRasterYSize();
    int width  = ds_mas->GetRasterXSize();
    GDALDataType datatype = rb_mas->GetRasterDataType();

    GDALDataset* ds_sla = (GDALDataset*)GDALOpen(argv[2], GA_ReadOnly);
    if(!ds_sla){
        return funcrst(false, "ds_sla is nullptr");
    }
    GDALRasterBand* rb_sla = ds_sla->GetRasterBand(1);

    GDALDriver* dr = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dr->Create(argv[3], width, height, 1, GDT_CFloat32, NULL);
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);


    complex<float>* arr_mas = new complex<float>[width];
    complex<float>* arr_sla = new complex<float>[width];
    complex<float>* arr_int = new complex<float>[width];
    for(int i=0; i< height; i++)
    {
        if(datatype == GDT_CFloat32)
        {
            rb_mas->RasterIO(GF_Read, 0, i, width, 1, arr_mas, width, 1, datatype, 0, 0);
            rb_sla->RasterIO(GF_Read, 0, i, width, 1, arr_sla, width, 1, datatype, 0, 0);
        }
        else if(datatype == GDT_CInt16)
        {
            complex<short>* t_arr = new complex<short>[width];
            rb_mas->RasterIO(GF_Read, 0, i, width, 1, t_arr, width, 1, datatype, 0, 0);
            for(int j = 0; j< width; j++){
                arr_mas[j] = t_arr[j];
            }
            rb_sla->RasterIO(GF_Read, 0, i, width, 1, t_arr, width, 1, datatype, 0, 0);
            for(int j = 0; j< width; j++){
                arr_sla[j] = t_arr[j];
            }
            delete[] t_arr;
        }

        for(int j = 0; j< width; j++){
            // arr_int[j] = arr_mas[j] * conj(arr_sla[j]);
            arr_int[j].real(arr_mas[j].real() * arr_sla[j].real() + arr_mas[j].imag() * arr_sla[j].imag());
            arr_int[j].imag(arr_mas[j].imag() * arr_sla[j].real() - arr_mas[j].real() * arr_sla[j].imag());
        }

        rb_out->RasterIO(GF_Write, 0, i, width, 1, arr_int, width, 1, GDT_CFloat32, 0, 0);
    }

    delete[] arr_int;
    delete[] arr_mas;
    delete[] arr_sla;

    GDALClose(ds_mas);
    GDALClose(ds_sla);
    GDALClose(ds_out);
    return return_msg(1, EXE_NAME " end.");
}
