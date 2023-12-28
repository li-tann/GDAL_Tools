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

#define EXE_NAME "interf_spectrum_filter"

// #define DEBUG

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum par_list{freq=0, bw, cnt_freq, inc};

struct slc_pars
{
    slc_pars(){}
    slc_pars(float freq, float bw, float cnt_freq, float inc)
        :frequency(freq),bandwidth(bw),center_frequency(cnt_freq),inc_rad(inc){}
    bool load(vector<string> str_vec){
        frequency = stof(str_vec[freq]);
        bandwidth = stof(str_vec[bw]);
        center_frequency = stof(str_vec[cnt_freq]);
        inc_rad = stof(str_vec[inc]);
    }
    
    float frequency;
    float bandwidth;
    float center_frequency;
    float inc_rad;
};



funcrst azimuth_filter(const char* mas_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* sla_out_path);
funcrst slant_range_filter(const char* int_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* out_path);

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

    if(argc < 6){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [params] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]:  input, master rslc filepath.\n"
                " argv[2]:  input, master par like: freq,bandwith,cnt_freq,inc.\n"
                " argv[3]:  input,  slave rslc filepath.\n"
                " argv[4]:  input,  slave par like: freq,bandwith,cnt_freq,inc.\n"
                " argv[5]:  input, method, azimuth or slant_range.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");
    vector<string> splited_str_vec;
    slc_pars mas_pars;
    strSplit(string(argv[2]), splited_str_vec, ",");
    if(splited_str_vec.size()<4)
        return return_msg(-1, "master splited_str_vec.size() < 4");
    mas_pars.load(splited_str_vec);

    splited_str_vec.clear();
    slc_pars sla_pars;
    strSplit(string(argv[4]), splited_str_vec, ",");
    if(splited_str_vec.size()<4)
        return return_msg(-1, "slave splited_str_vec.size() < 4");
    sla_pars.load(splited_str_vec);

    string method(argv[5]);
    funcrst rst;
    if(method == "azimuth")
    {
        rst = azimuth_filter(argv[1], argv[3], mas_pars, sla_pars, argv[3]);
    }
    else if(method == "slant_range")
    {
        rst = slant_range_filter(argv[1], argv[3], mas_pars, sla_pars, argv[3]);
    }
    else{
        return_msg(-1, "unknown method.");
    }



    return return_msg(1, EXE_NAME " end.");
}

funcrst azimuth_filter(const char* mas_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* sla_out_path)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_mas = (GDALDataset*)GDALOpen(mas_path, GA_ReadOnly);
    if(!ds_mas){
        return funcrst(false, "ds_mas is nullptr");
    }
    GDALRasterBand* rb_mas = ds_mas->GetRasterBand(1);

    int height = ds_mas->GetRasterYSize();
    int width  = ds_mas->GetRasterXSize();
    GDALDataType datatpe = rb_mas->GetRasterDataType();

    GDALDataset* ds_sla = (GDALDataset*)GDALOpen(sla_path, GA_ReadOnly);
    if(!ds_sla){
        return funcrst(false, "ds_sla is nullptr");
    }
    GDALRasterBand* rb_sla = ds_sla->GetRasterBand(1);

    /// slave 的宽, 高, 数据类型就不在读取, 默认与主影像相同


    /// 读取影像的行, fft转换,  生成矩形窗口, 滤波, 反转换, 说出波形的txt


    GDALClose(ds_mas);
    return funcrst(true, "azimuth_filter success.");
}


funcrst slant_range_filter(const char* mas_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* sla_out_path)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");






    return funcrst(true, "slant_range_filter success.");
}
