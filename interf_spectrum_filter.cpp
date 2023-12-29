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
        if(str_vec.size() < 4)
            return false;
        frequency = stof(str_vec[freq]);
        bandwidth = stof(str_vec[bw]);
        center_frequency = stof(str_vec[cnt_freq]);
        inc_rad = stof(str_vec[inc]) * M_PI / 180;
        return true;
    }
    
    float frequency;
    float bandwidth;
    float center_frequency;
    float inc_rad;
};



funcrst azimuth_filter(const char* mas_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* sla_out_path);
funcrst slant_range_filter(const char* int_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* mas_out_path, const char* sla_out_path);

funcrst slant_range_filter_line(const char* int_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars);

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
                " argv[5]:  input, method, az, azline, rg, or rgline.";
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
    if(method == "az")
    {
        rst = azimuth_filter(argv[1], argv[3], mas_pars, sla_pars, argv[3]);
    }
    else if(method == "az_line")
    {
        rst = azimuth_filter(argv[1], argv[3], mas_pars, sla_pars, argv[3]);
    }
    else if(method == "rg")
    {
        string mas_out = argv[1];
        string sla_out = argv[3];
        mas_out.insert(mas_out.find_last_of("."),".fil");
        sla_out.insert(sla_out.find_last_of("."),".fil");
        rst = slant_range_filter(argv[1], argv[3], mas_pars, sla_pars, mas_out.c_str(), sla_out.c_str());
    }
    else if(method == "rg_line")
    {
        rst = slant_range_filter_line(argv[1], argv[3], mas_pars, sla_pars);
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
    GDALDataType datatype = rb_mas->GetRasterDataType();

    GDALDataset* ds_sla = (GDALDataset*)GDALOpen(sla_path, GA_ReadOnly);
    if(!ds_sla){
        return funcrst(false, "ds_sla is nullptr");
    }
    GDALRasterBand* rb_sla = ds_sla->GetRasterBand(1);

    /// slave 的宽, 高, 数据类型就不再读取, 默认与主影像相同


    /// 计算delta_freq


    /// 生成频域的矩形窗口
    float* widght = new float[height];
    for(int i=0; i< height; i++){
        float i_freq = i / height * freq;
        widght[i] = 1;
    }

    /// 读取影像的行, fft转换,  生成矩形窗口, 滤波, 反转换, 说出波形的txt
    complex<float>* arr_mas = new complex<float>[height];
    complex<float>* arr_sla = new complex<float>[height];
    if(datatype == GDT_CFloat32)
    {
        rb_mas->RasterIO(GF_Read, 0, 100, width, 1, arr_mas, width, 1, datatype, 0, 0);
        rb_sla->RasterIO(GF_Read, 0, 100, width, 1, arr_sla, width, 1, datatype, 0, 0);
    }
    else if(datatype == GDT_CInt16)
    {
        complex<short>* t_arr = new complex<short>[height];
        rb_mas->RasterIO(GF_Read, 0, 100, width, 1, t_arr, width, 1, datatype, 0, 0);
        for(int i = 0; i< height; i++){
            arr_mas[i] = t_arr[i];
        }
        rb_sla->RasterIO(GF_Read, 0, 100, width, 1, t_arr, width, 1, datatype, 0, 0);
        for(int i = 0; i< height; i++){
            arr_sla[i] = t_arr[i];
        }
        delete[] t_arr;
    }

    fftwf_complex* spectral_arr = fftwf_alloc_complex(height);
    fftwf_complex* frequency_arr = fftwf_alloc_complex(height);
    fftwf_plan s_to_f = fftwf_plan_dft_1d(height, spectral_arr, frequency_arr, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_plan f_to_s = fftwf_plan_dft_1d(height, frequency_arr, spectral_arr, FFTW_BACKWARD, FFTW_ESTIMATE);

    ofstream ofs("azimuth_line.txt");
    if(!ofs.is_open()){
        return funcrst(false, "ofs open failed.");
    }
    /// mas s -> f -> widght -> s
    for(int i=0; i<height; i++){
        spectral_arr[i][0] = arr_mas[i].real();
        spectral_arr[i][1] = arr_mas[i].imag();
    }
    fftwf_execute(s_to_f);

    /// write
    ofs<<"azimuth, ";
    for(int i=0; i<height; i++){
        spectral_arr[i][0] = arr_mas[i].real();
        spectral_arr[i][1] = arr_mas[i].imag();
    }

    for(int i = 0; i < height; i++){
        frequency_arr[i][0] *= widght[i];
        frequency_arr[i][1] *= widght[i];
    }
    fftwf_execute(f_to_s);


    delete[] arr_mas;
    delete[] arr_sla;


    GDALClose(ds_mas);
    GDALClose(ds_sla);
    return funcrst(true, "azimuth_filter success.");
}

funcrst slant_range_filter(const char* mas_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars , const char* mas_out_path, const char* sla_out_path)
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
    GDALDataType datatype = rb_mas->GetRasterDataType();

    GDALDataset* ds_sla = (GDALDataset*)GDALOpen(sla_path, GA_ReadOnly);
    if(!ds_sla){
        return funcrst(false, "ds_sla is nullptr");
    }
    GDALRasterBand* rb_sla = ds_sla->GetRasterBand(1);

    /// slave 的宽, 高, 数据类型就不再读取, 默认与主影像相同


    /// 计算delta_freq
    float delta_freq_mas = -mas_pars.center_frequency * (mas_pars.inc_rad - sla_pars.inc_rad) / tan(mas_pars.inc_rad);
    delta_freq_mas *= -1;
    // delta_freq_mas *= 2;
    float delta_freq_sla = -delta_freq_mas;
    float freq_range_mas_min = delta_freq_mas < 0 ? mas_pars.bandwidth/2 + delta_freq_mas       : mas_pars.bandwidth/2;
    float freq_range_mas_max = delta_freq_mas < 0 ? mas_pars.frequency - mas_pars.bandwidth/2   : mas_pars.frequency - mas_pars.bandwidth/2 + delta_freq_mas;

    float freq_range_sla_min = delta_freq_sla < 0 ? sla_pars.bandwidth/2 + delta_freq_sla       : sla_pars.bandwidth/2;
    float freq_range_sla_max = delta_freq_sla < 0 ? sla_pars.frequency - sla_pars.bandwidth/2   : sla_pars.frequency - sla_pars.bandwidth/2 + delta_freq_sla;

    std::cout<<fmt::format("bw:{}, freq:{}, bw_pix:{}, freq_pix:{}\n",
                mas_pars.bandwidth,
                mas_pars.frequency,
                mas_pars.bandwidth * width /  mas_pars.frequency,
                mas_pars.frequency * width /  mas_pars.frequency);

    /// 生成频域的矩形窗口
    float* weight_mas = new float[width];
    float* weight_sla = new float[width];
    int zero_num = 0;
    for(int i=0; i< width; i++){
        float freq_i = mas_pars.frequency * i / width;
        weight_mas[i] = (freq_i < freq_range_mas_max && freq_i > freq_range_mas_min) ? 0. : 1.;
        weight_sla[i] = (freq_i < freq_range_sla_max && freq_i > freq_range_sla_min) ? 0. : 1.;
        if(weight_mas[i] == 0) zero_num++;
        // if(weight_sla[i] == 0) zero_num++;
    }
    printf("zero_num_mas: %d/%d\n", zero_num, width);

    /// 读取影像的行, fft转换,  生成矩形窗口, 滤波, 反转换, 说出波形的txt
    complex<float>* arr_mas = new complex<float>[width];
    complex<float>* arr_sla = new complex<float>[width];

    fftwf_complex* spectral_arr = fftwf_alloc_complex(width);
    fftwf_complex* frequency_arr = fftwf_alloc_complex(width);
    fftwf_plan s_to_f = fftwf_plan_dft_1d(width, spectral_arr, frequency_arr, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_plan f_to_s = fftwf_plan_dft_1d(width, frequency_arr, spectral_arr, FFTW_BACKWARD, FFTW_ESTIMATE);

    GDALDriver* dr = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_mas_out = dr->Create(mas_out_path, width, height, 1, GDT_CFloat32, NULL);
    GDALRasterBand* rb_mas_out = ds_mas_out->GetRasterBand(1);
    GDALDataset* ds_sla_out = dr->Create(sla_out_path, width, height, 1, GDT_CFloat32, NULL);
    GDALRasterBand* rb_sla_out = ds_sla_out->GetRasterBand(1);

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
            for(int i = 0; i< width; i++){
                arr_mas[i] = t_arr[i];
            }
            rb_sla->RasterIO(GF_Read, 0, i, width, 1, t_arr, width, 1, datatype, 0, 0);
            for(int i = 0; i< width; i++){
                arr_sla[i] = t_arr[i];
            }
            delete[] t_arr;
        }

        /// mas s -> f -> widght -> s
        for(int i=0; i<width; i++){
            spectral_arr[i][0] = arr_mas[i].real();
            spectral_arr[i][1] = arr_mas[i].imag();
        }
        fftwf_execute(s_to_f);
        for(int i = 0; i < width; i++){
            frequency_arr[i][0] *= weight_mas[i];
            frequency_arr[i][1] *= weight_mas[i];
        }
        fftwf_execute(f_to_s);
        for(int i=0; i<width; i++){
            arr_mas[i].real(spectral_arr[i][0] / width);
            arr_mas[i].imag(spectral_arr[i][1] / width);
        }

        /// sla s -> f -> widght -> s
        for(int i=0; i<width; i++){
            spectral_arr[i][0] = arr_sla[i].real();
            spectral_arr[i][1] = arr_sla[i].imag();
        }
        fftwf_execute(s_to_f);
        for(int i = 0; i < width; i++){
            frequency_arr[i][0] *= weight_sla[i];
            frequency_arr[i][1] *= weight_sla[i];
        }
        fftwf_execute(f_to_s);
        for(int i=0; i<width; i++){
            arr_sla[i].real(spectral_arr[i][0] / width);
            arr_sla[i].imag(spectral_arr[i][1] / width);
        }

        rb_mas_out->RasterIO(GF_Write, 0, i, width, 1, arr_mas, width, 1, GDT_CFloat32, 0, 0);
        rb_sla_out->RasterIO(GF_Write, 0, i, width, 1, arr_sla, width, 1, GDT_CFloat32, 0, 0);
    }

    delete[] arr_mas;
    delete[] arr_sla;
    delete[] weight_mas;
    delete[] weight_sla;

    fftwf_destroy_plan(s_to_f);
    fftwf_destroy_plan(f_to_s);
    fftwf_free(spectral_arr);
    fftwf_free(frequency_arr);


    GDALClose(ds_mas);
    GDALClose(ds_sla);
    GDALClose(ds_mas_out);
    GDALClose(ds_sla_out);
    return funcrst(true, "slant_range_filter success.");
}



funcrst slant_range_filter_line(const char* mas_path, const char* sla_path, slc_pars mas_pars, slc_pars sla_pars)
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
    GDALDataType datatype = rb_mas->GetRasterDataType();

    GDALDataset* ds_sla = (GDALDataset*)GDALOpen(sla_path, GA_ReadOnly);
    if(!ds_sla){
        return funcrst(false, "ds_sla is nullptr");
    }
    GDALRasterBand* rb_sla = ds_sla->GetRasterBand(1);

    /// slave 的宽, 高, 数据类型就不再读取, 默认与主影像相同


    /// 计算delta_freq
    float delta_freq_mas = -mas_pars.center_frequency * (mas_pars.inc_rad - sla_pars.inc_rad) / tan(mas_pars.inc_rad);
    // delta_freq_mas *= -1;
    float delta_freq_sla = -delta_freq_mas;
    float freq_range_mas_min = delta_freq_mas < 0 ? mas_pars.bandwidth/2 + delta_freq_mas       : mas_pars.bandwidth/2;
    float freq_range_mas_max = delta_freq_mas < 0 ? mas_pars.frequency - mas_pars.bandwidth/2   : mas_pars.frequency - mas_pars.bandwidth/2 + delta_freq_mas;
    
    std::cout<<fmt::format("delta_freq: {}, freq_range_mas_min: {}, freq_range_mas_max: {}", delta_freq_mas, freq_range_mas_min, freq_range_mas_max)<<endl;
    std::cout<<fmt::format("delta_freq: {}, freq_range_mas_min: {}, freq_range_mas_max: {}", 
                    delta_freq_mas * width / mas_pars.frequency, 
                    freq_range_mas_min * width / mas_pars.frequency, 
                    freq_range_mas_max * width / mas_pars.frequency)
                    <<endl;

    float freq_range_sla_min = delta_freq_sla < 0 ? sla_pars.bandwidth/2 + delta_freq_sla       : sla_pars.bandwidth/2;
    float freq_range_sla_max = delta_freq_sla < 0 ? sla_pars.frequency - sla_pars.bandwidth/2   : sla_pars.frequency - sla_pars.bandwidth/2 + delta_freq_sla;
    std::cout<<fmt::format("delta_freq: {}, freq_range_sla_min: {}, freq_range_sla_max: {}", delta_freq_sla, freq_range_mas_min, freq_range_mas_max)<<endl;
    std::cout<<fmt::format("delta_freq: {}, freq_range_sla_min: {}, freq_range_sla_max: {}", 
                    delta_freq_sla * width / mas_pars.frequency, 
                    freq_range_sla_min * width / mas_pars.frequency, 
                    freq_range_sla_max * width / mas_pars.frequency)
                    <<endl;

    std::cout<<fmt::format("bw:{}, freq:{}, bw_pix:{}, freq_pix:{}\n",
                mas_pars.bandwidth,
                mas_pars.frequency,
                mas_pars.bandwidth * width /  mas_pars.frequency,
                mas_pars.frequency * width /  mas_pars.frequency);
    /// 生成频域的矩形窗口
    float* weight_mas = new float[width];
    float* weight_sla = new float[width];
    int zero_num = 0;
    for(int i=0; i< width; i++){
        float freq_i = mas_pars.frequency * i / width;
        weight_mas[i] = (freq_i < freq_range_mas_max && freq_i > freq_range_mas_min) ? 0. : 1.;
        weight_sla[i] = (freq_i < freq_range_sla_max && freq_i > freq_range_sla_min) ? 0. : 1.;
        if(weight_mas[i] == 0) zero_num++;
        // if(weight_sla[i] == 0) zero_num++;
    }
    printf("zero_num_mas: %d/%d\n", zero_num, width);

    /// 读取影像的行, fft转换,  生成矩形窗口, 滤波, 反转换, 说出波形的txt
    complex<float>* arr_mas = new complex<float>[width];
    complex<float>* arr_sla = new complex<float>[width];
    if(datatype == GDT_CFloat32)
    {
        rb_mas->RasterIO(GF_Read, 0, 100, width, 1, arr_mas, width, 1, datatype, 0, 0);
        rb_sla->RasterIO(GF_Read, 0, 100, width, 1, arr_sla, width, 1, datatype, 0, 0);
    }
    else if(datatype == GDT_CInt16)
    {
        complex<short>* t_arr = new complex<short>[width];
        rb_mas->RasterIO(GF_Read, 0, 100, width, 1, t_arr, width, 1, datatype, 0, 0);
        for(int i = 0; i< width; i++){
            arr_mas[i] = t_arr[i];
        }
        rb_sla->RasterIO(GF_Read, 0, 100, width, 1, t_arr, width, 1, datatype, 0, 0);
        for(int i = 0; i< width; i++){
            arr_sla[i] = t_arr[i];
        }
        delete[] t_arr;
    }

    complex<float>* arr_int = new complex<float>[width];
    for(int i=0; i< width; i++){
        arr_int[i] = arr_mas[i] * conj(arr_sla[i]); 
    }

    fftwf_complex* spectral_arr = fftwf_alloc_complex(width);
    fftwf_complex* frequency_arr = fftwf_alloc_complex(width);
    fftwf_plan s_to_f = fftwf_plan_dft_1d(width, spectral_arr, frequency_arr, FFTW_FORWARD, FFTW_ESTIMATE);
    fftwf_plan f_to_s = fftwf_plan_dft_1d(width, frequency_arr, spectral_arr, FFTW_BACKWARD, FFTW_ESTIMATE);

    ofstream ofs("slant_range_line.txt");
    if(!ofs.is_open()){
        return funcrst(false, "ofs open failed.");
    }
    /// write
    ofs<<"weight_mas\n";
    for(int i=0; i<width; i++){
        ofs<<weight_mas[i]<<",";
    }
    ofs<<"\n";
    ofs<<"weight_sla\n";
    for(int i=0; i<width; i++){
        ofs<<weight_sla[i]<<",";
    }
    ofs<<"\n";

    /// write
    ofs<<"int phase, before filter\n";
    for(int i=0; i<width; i++){
        ofs<<atan2f(arr_int[i].imag(), arr_int[i].real())<<",";
    }
    ofs<<"\n";


    /// mas s -> f -> widght -> s
    for(int i=0; i<width; i++){
        spectral_arr[i][0] = arr_mas[i].real();
        spectral_arr[i][1] = arr_mas[i].imag();
    }
    fftwf_execute(s_to_f);

    /// write
    ofs<<"mas freq.abs, before weight\n";
    for(int i=0; i<width; i++){
        ofs<<sqrt(powf(frequency_arr[i][0],2)+powf(frequency_arr[i][1],2))<<",";
    }
    ofs<<"\n";
    for(int i = 0; i < width; i++){
        frequency_arr[i][0] *= weight_mas[i];
        frequency_arr[i][1] *= weight_mas[i];
    }
    fftwf_execute(f_to_s);
    /// write
    ofs<<"mas freq.abs, after weight\n";
    for(int i=0; i<width; i++){
        ofs<<sqrt(powf(frequency_arr[i][0],2)+powf(frequency_arr[i][1],2))<<",";
    }
    ofs<<"\n";

    for(int i=0; i<width; i++){
        arr_mas[i].real(spectral_arr[i][0] / width);
        arr_mas[i].imag(spectral_arr[i][1] / width);
    }

    /// sla s -> f -> widght -> s
    for(int i=0; i<width; i++){
        spectral_arr[i][0] = arr_sla[i].real();
        spectral_arr[i][1] = arr_sla[i].imag();
    }
    fftwf_execute(s_to_f);
    /// write
    ofs<<"mas freq.abs, before weight\n";
    for(int i=0; i<width; i++){
        ofs<<sqrt(powf(frequency_arr[i][0],2)+powf(frequency_arr[i][1],2))<<",";
    }
    ofs<<"\n";
    for(int i = 0; i < width; i++){
        frequency_arr[i][0] *= weight_sla[i];
        frequency_arr[i][1] *= weight_sla[i];
    }
    fftwf_execute(f_to_s);
    /// write
    ofs<<"mas freq.abs, after weight\n";
    for(int i=0; i<width; i++){
        ofs<<sqrt(powf(frequency_arr[i][0],2)+powf(frequency_arr[i][1],2))<<",";
    }
    ofs<<"\n";
    for(int i=0; i<width; i++){
        arr_sla[i].real(spectral_arr[i][0] / width);
        arr_sla[i].imag(spectral_arr[i][1] / width);
    }

    for(int i=0; i< width; i++){
        arr_int[i] = arr_mas[i] * conj(arr_sla[i]); 
    }

    /// write
    ofs<<"int phase, after filter\n";
    for(int i=0; i<width; i++){
        ofs<<atan2f(arr_int[i].imag(), arr_int[i].real())<<",";
    }
    ofs<<"\n";


    ofs.close();
    delete[] arr_mas;
    delete[] arr_sla;
    delete[] arr_int;
    delete[] weight_mas;
    delete[] weight_sla;

    fftwf_destroy_plan(s_to_f);
    fftwf_destroy_plan(f_to_s);
    fftwf_free(spectral_arr);
    fftwf_free(frequency_arr);


    GDALClose(ds_mas);
    GDALClose(ds_sla);
    return funcrst(true, "slant_range_filter success.");
}
