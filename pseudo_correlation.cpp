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

#include "datatype.h"

#define EXE_NAME "pseudo_correlation"

// #define DEBUG

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

float warp(float src)
{
    while (src > M_PI) { src -= 2 * M_PI; }
    while (src < -M_PI) { src += 2 * M_PI; }
    return src;
}

float norm(complex<float> src){
    return sqrtf(powf(src.real(),2)+powf(src.imag(),2));
}

/// 测试一下传统计算相干性的耗时, 仅用于对比测试
double class_pseudo_correlation(complex<float>* arr, float* arr_out, int height, int width, int size);
double my_pseudo_correlation(complex<float>* arr, float* arr_out, int height, int width, int size);

int main(int argc, char* argv[])
{
    string msg;
    std::mutex mtx;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [params] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, fcpx filepath.\n"
                " argv[2]: input, win_size, like 3,5,7,... .\n"
                " argv[3]: output, pesudo correlation filepath(flt).";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

	int size = (float)atof(argv[2]);
    int size_new = size/2*2+1;
    if(size_new != size){
        return_msg(1,fmt::format("valid size is {}(origin input is {})",size_new, size));
        size = size_new;
    }
    
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
		return return_msg(-2, "datatype is diff with fcomplex.");
	}

    /// got warped phase
	complex<float>* arr = new complex<float>[width * height];
    rb->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);
    GDALClose(ds);
	

    /// cal pseudo correlation
	float* arr_out = new float[width * height];

    double cpc_spent_sec = class_pseudo_correlation(arr, arr_out, height, width, size);
    cout<<"cpc_spent_sec: "<<cpc_spent_sec<<"s\n";

    double mpc_spent_sec = my_pseudo_correlation(arr, arr_out, height, width, size);
    cout<<"mpc_spent_sec: "<<mpc_spent_sec<<"s\n";

    /// write arr_out
	GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(argv[3], width, height, 1, GDT_Float32, NULL);
    if(!ds_out){
		delete[] arr_out;
        return return_msg(-3, "ds_out create failed.");
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);

	rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, GDT_Float32, 0, 0);

	delete[] arr_out;
	GDALClose(ds_out);

    return return_msg(1, EXE_NAME " end.");
}

/// 测试一下传统计算相干性的耗时
double class_pseudo_correlation(complex<float>* arr, float* arr_out, int height, int width, int size)
{
    auto st = chrono::system_clock::now();
    int num=0;
#pragma omp parallel for
    for(int i=0; i<height; i++)
    {
        cout<<fmt::format("\rpercentage: {}/{}...",num++,height);
        int start_row = MAX(0,i-size/2);
        int end_row = MIN(i+size/2,height-1);
        int win_height= end_row - start_row + 1;
        /// sum 即总和, abs_sum即模长的总和, left即最左侧一列的总和, abs_left即最左侧一列绝对值的总和, right与abs_right同理
        
        for(int j=0; j<width; j++)
        {
            int start_col = MAX(0,j-size/2);
            int end_col = MIN(j+size/2,width-1);
            complex<float> sum(0,0);
            float abs_sum=0;
            for(int ii = start_row; ii < end_row; ii++)
            {
                for(int jj = start_col; jj < end_col; jj++)
                {
                    sum += arr[ii * width + jj];
                    abs_sum += abs(arr[ii * width + jj]);
                }
            }
            arr_out[i*width + 0] = (abs_sum == 0) ? 0 : abs(sum)/abs_sum;
        }
    }
    cout<<endl;

    return spend_time(st);
}

double my_pseudo_correlation(complex<float>* arr, float* arr_out, int height, int width, int size)
{
    auto st = chrono::system_clock::now();
    int num=0;
#pragma omp parallel for
    for(int i=0; i<height; i++)
    {
        cout<<fmt::format("\rpercentage: {}/{}...",num++,height);
        int start_row = MAX(0,i-size/2);
        int end_row = MIN(i+size/2,height-1);
        int win_height= end_row - start_row + 1;
        /// sum 即总和, abs_sum即模长的总和, left即最左侧一列的总和, abs_left即最左侧一列绝对值的总和, right与abs_right同理
        complex<float> sum(0,0), left(0,0), right(0,0);
        float abs_sum=0,abs_left=0, abs_right=0;

        /// j=0
        for(int k = start_row; k<= end_row; k++)
        {
            for(int j=0; j<size/2+1; j++){
                sum += arr[k * width + j];
                abs_sum += abs(arr[k * width + j]);
            }
        }
        arr_out[i*width + 0] = (abs_sum == 0) ? 0 : abs(sum)/abs_sum;

        /// j=1~size/2
        for(int j=1; j<=size/2; j++)
        {
            right=0;
            abs_right=0;
            int j_r=j + size / 2;
            for(int k = start_row; k<= end_row; k++){
                right += arr[k * width + j_r];
                abs_right += abs(arr[k * width + j_r]);
            }
            sum += right;
            abs_sum += abs_right;
            arr_out[i*width + j] = abs_sum == 0 ? 0 : abs(sum)/abs_sum;
        }

        /// j=size/2~width-size/2
        for(int j=size/2+1; j<width-size/2; j++)
        {
            right=0;
            abs_right=0;
            left=0;
            abs_left=0;
            int j_r=j + size / 2;
            int j_l=j - size / 2 - 1;
            for(int k = start_row; k<= end_row; k++)
            {
                right += arr[k * width + j_r];
                abs_right += abs(arr[k * width + j_r]);
                left += arr[k * width + j_l];
                abs_left += abs(arr[k * width + j_l]);
            }
            sum += right - left;
            abs_sum += abs_right - abs_left;
            arr_out[i*width + j] = abs_sum == 0 ? 0 : abs(sum)/abs_sum;
        }

        /// j=width-size/2 ~ width-1
        for(int j=width-size/2; j<width; j++){
            left=0;
            abs_left=0;
            int j_l=j - size / 2 - 1;
            for(int k = start_row; k<= end_row; k++)
            {
                left += arr[k * width + j_l];
                abs_left += abs(arr[k * width + j_l]);
            }
            sum -= left;
            abs_sum -= abs_left;
            arr_out[i*width + j] = abs_sum == 0 ? 0 : abs(sum)/abs_sum;
        }
    }
    cout<<endl;

    return spend_time(st);
}