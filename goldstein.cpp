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

#define EXE_NAME "goldstein"

// #define DEBUG

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

funcrst goldstein(std::complex<float>* arr_in, int height, int width, float alpha, std::complex<float>* arr_out);
funcrst goldstein_single(std::complex<float>* arr_in, int height, int width, float alpha, std::complex<float>* arr_out);

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

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [params] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, flt/fcpx filepath.\n"
                " argv[2]: input, parameter about alpha.\n"
                " argv[3]: output, flt/fcpx filepath.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

	float alpha = (float)atof(argv[2]);

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
		return return_msg(-2, "datatype is diff with float(not support yet) or fcomplex.");
	}

	std::complex<float>* arr = new std::complex<float>[width * height];
	std::complex<float>* arr_out = new std::complex<float>[width * height];
	rb->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);
	goldstein_single(arr, height, width, alpha, arr_out);
    // goldstein(arr, height, width, alpha, arr_out);

    cout<<"before :"<<arr[40000]<<endl;
    cout<<"after  :"<<arr_out[40000]<<endl;

    delete[] arr;
	GDALClose(ds);

	GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(argv[3], width, height, 1, datatype, NULL);
    if(!ds_out){
		delete[] arr_out;
        return return_msg(-3, "ds_out create failed.");
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);

	rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, datatype, 0, 0);

	delete[] arr_out;
	GDALClose(ds_out);
    
    return return_msg(1, EXE_NAME " end.");
}

funcrst conv_2d(float* arr_in, int width, int height, float* arr_out, float* kernel, int size)
{
	if(arr_in == nullptr)
		return funcrst(false, "filter::conv_2d, arr_in is nullptr.");
	
	if(dynamic_array_size(arr_in) != width * height)
		return funcrst(false, fmt::format("filter::conv_2d, arr_in.size({}) is diff with width*height({}).",dynamic_array_size(arr_in),width * height));
	
	if(kernel == nullptr)
		return funcrst(false, "filter::conv_2d, kernel is nullptr.");

	if(dynamic_array_size(kernel) != size * size)
		return funcrst(false, fmt::format("filter::conv_2d, kernel.size({}) is diff with size^2({}).",dynamic_array_size(kernel),size*size));

	float* kernel_overturn = new float[size*size];
	for(int i=0; i<size*size; i++)
		kernel_overturn[i] = kernel[size*size-1-i];

	if(arr_out == nullptr){
		arr_out = new float[height * width];
	}
	else if(dynamic_array_size(arr_out) != height * width){
		delete[] arr_out;
		arr_out = new float[height * width];
	}

	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			/// 这种重复计算的方式肯定会多耗费一些时间, 如果使用同行向右滑动, 逐列增减数据的方式, 可以大大减少耗时
			float sum = 0;
			for(int m = 0; m< size; m++){
				for(int n = 0; n< size; n++){
					if(i-size/2+m < 0 || i-size/2+m > height-1 || j-size/2+n < 0 || j-size/2+n > width-1)
						continue;/// 超界
					sum +=  kernel_overturn[m*size+n] * arr_in[(i-size/2+m)*width+(j-size/2+n)];
				}
			}
			arr_out[i*width+j]=sum;
		}
	}

	delete[] kernel_overturn;
	return funcrst(true, "filter::conv_2d finished.");
}


funcrst goldstein(std::complex<float>* arr_in, int height, int width, float alpha, std::complex<float>* arr_out)
{

    auto start_time = std::chrono::system_clock::now();

	int size = 32;
	int overlap = 24;
	int step = size - overlap;

	int max_threads = omp_get_max_threads();
    cout<<"max_threads: "<<max_threads<<endl;

	/// 避免多线程时多线程同时调用一个plan出现异常
	fftwf_plan* fftw_plans = new fftwf_plan[max_threads];
	fftwf_plan* ifftw_plans = new fftwf_plan[max_threads];
	fftwf_complex** spatial_arrs  = new fftwf_complex*[max_threads];
	fftwf_complex** frequency_arrs = new fftwf_complex*[max_threads];
	for(int i=0; i<max_threads; i++){
		spatial_arrs[i]  = fftwf_alloc_complex(size*size);
		frequency_arrs[i] = fftwf_alloc_complex(size*size);
		fftw_plans[i]  = fftwf_plan_dft_2d(size, size, spatial_arrs[i], frequency_arrs[i], FFTW_FORWARD, FFTW_ESTIMATE);
		ifftw_plans[i] = fftwf_plan_dft_2d(size, size, frequency_arrs[i], spatial_arrs[i], FFTW_BACKWARD, FFTW_ESTIMATE);
	}

	if(arr_out == nullptr){
		arr_out = new std::complex<float>[height * width];
	}
	else if(dynamic_array_size(arr_out) != height * width){
		delete[] arr_out;
		arr_out = new std::complex<float>[height * width];
	}

	/// smooth
	float* smooth_spatial = fftwf_alloc_real(25);
	// float* smooth_frequency = fftwf_alloc_real(25);
	for(int i=0;i<25;i++) 
        smooth_spatial[i] = 0.04;
	// fftwf_plan smooth_fft_plan = fftwf_plan_r2r_2d( 5, 5, smooth_spatial, smooth_frequency, FFTW_R2HC, FFTW_R2HC, FFTW_MEASURE);
	// fftwf_execute(smooth_fft_plan);
	// fftwf_destroy_plan(smooth_fft_plan);
	// delete[] smooth_spatial;

#pragma omp parallel for
	for(int i=0; i < height; i+=step)
	{
		/// out_i_start, out_i_end, 控制block数组内需要赋值到arr_out的行数, 保证输出数据没有"黑框"
		int out_i_start, out_i_end;
		if(i == 0){
			out_i_start = 0; out_i_end = size - overlap / 2 - 1;
		}
		else if(i + step - 1 > height - 1){
			out_i_start = overlap / 2; out_i_end = height - 1 - i;
		}
		else{
			out_i_start = overlap / 2; out_i_end = size - overlap / 2 - 1;
		}
		
		int thread_idx = omp_get_thread_num();
        cout<<"current thread idx: "<<thread_idx<<endl;
		for(int j=0; j < width; j+=step)
		{
			/// out_j_start, out_j_end, 控制block数组内需要赋值到arr_out的列数, 保证输出数据没有"黑框"
			int out_j_start, out_j_end;
			if(j == 0){
				out_j_start = 0; out_j_end = size - overlap / 2 - 1;
			}
			else if(j + step - 1 > width - 1){
				out_j_start = overlap / 2; out_j_end = width - 1 - j;
			}
			else{
				out_j_start = overlap / 2; out_j_end = size - overlap / 2 - 1;
			}

			/// spatial_arrs init
			for(size_t k = 0; k< size*size; k++){
				int block_i = k / size + i;
				int block_j = k % size + j;
				if(block_j > width - 1 || block_i > height - 1){
					/// 说明超界, 需要补零
					spatial_arrs[thread_idx][k][0]=0;
					spatial_arrs[thread_idx][k][1]=0;
				}
				else{
					spatial_arrs[thread_idx][k][0]=arr_in[block_i * width + block_j].real();
					spatial_arrs[thread_idx][k][1]=arr_in[block_i * width + block_j].imag();
				}
			}

			///  fft
			fftwf_execute(fftw_plans[thread_idx]);

			/// abs
			float* block_abs = new float[size*size];
			for(int k=0; k<size*size; k++){
				block_abs[k] = sqrtf(powf(frequency_arrs[thread_idx][k][0],2) + powf(frequency_arrs[thread_idx][k][1],2));
			}

			/// smooth
			float* block_smooth = new float[size*size];
			// conv_2d(block_abs, size, size, block_smooth, smooth_frequency, 5);
            conv_2d(block_abs, size, size, block_smooth, smooth_spatial, 5);
			delete[] block_abs;

			/// *alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
				frequency_arrs[thread_idx][k][0] = block_smooth[k] * alpha * spatial_arrs[thread_idx][k][0];
				frequency_arrs[thread_idx][k][1] = block_smooth[k] * alpha * spatial_arrs[thread_idx][k][1];
			}

			/// ifft
			fftwf_execute(ifftw_plans[thread_idx]);

			/// 赋值
			for(int m = out_i_start; m <= out_i_end; m++){
				for(int n = out_j_start; n <= out_j_end; n++){
                    if(i + m < 0 || i + m > height - 1 || j + n < 0 || j + n > width - 1)
                        continue;
					arr_out[(i+m)*width+(j+n)].real(frequency_arrs[thread_idx][m*size+n][0]);
					arr_out[(i+m)*width+(j+n)].imag(frequency_arrs[thread_idx][m*size+n][1]);
				}
			}


		}
	}


	for(int i=0; i<max_threads; i++){
		fftwf_destroy_plan(fftw_plans[i]);
        fftwf_destroy_plan(ifftw_plans[i]);
        fftwf_free(spatial_arrs[i]);
        fftwf_free(frequency_arrs[i]);
	}

    double spend_sec = spend_time(start_time);
    cout<<"glodstein spend_time: "<<spend_sec<<endl;

	return funcrst(true, "filter::goldstein finished.");
}


funcrst goldstein_single(std::complex<float>* arr_in, int height, int width, float alpha, std::complex<float>* arr_out)
{
    auto start_time = std::chrono::system_clock::now();

	int size = 32;
	int overlap = 24;
	int step = size - overlap;

	/// 避免多线程时多线程同时调用一个plan出现异常
    fftwf_complex* spatial_arr =  fftwf_alloc_complex(size*size);
	fftwf_complex* frequency_arr = fftwf_alloc_complex(size*size);
	fftwf_plan forward  = fftwf_plan_dft_2d(size, size, spatial_arr, frequency_arr, FFTW_FORWARD, FFTW_ESTIMATE);
	fftwf_plan backward = fftwf_plan_dft_2d(size, size, frequency_arr, spatial_arr, FFTW_BACKWARD, FFTW_ESTIMATE);

	if(arr_out == nullptr){
		arr_out = new std::complex<float>[height * width];
	}
	else if(dynamic_array_size(arr_out) != height * width){
		delete[] arr_out;
		arr_out = new std::complex<float>[height * width];
	}

    auto print_fftwf_arr = [](fftwf_complex* arr, int size){
        cout<<endl;
        for(int i=0; i< size; i++){
            cout<<arr[i][0]<<","<<arr[i][1]<<";";
        }
        cout<<endl;
    };

    auto print_fcpx_arr = [](std::complex<float>* arr, int size){
        cout<<endl;
        for(int i=0; i< size; i++){
            cout<<arr[i].real()<<","<<arr[i].imag()<<";";
        }
        cout<<endl;
    };

    auto print_flt_arr = [](float* arr, int size){
        cout<<endl;
        for(int i=0; i< size; i++){
            cout<<arr[i]<<",";
        }
        cout<<endl;
    };

	/// smooth
	float* smooth_spatial = new float[25];
	float* smooth_frequency = new float[25];
	for(int i=0;i<25;i++)
        smooth_spatial[i] = 0.04;
	// fftwf_plan smooth_fft_plan = fftwf_plan_r2r_2d( 5, 5, smooth_spatial, smooth_frequency, FFTW_R2HC, FFTW_R2HC, FFTW_MEASURE);
	// fftwf_execute(smooth_fft_plan);
	// fftwf_destroy_plan(smooth_fft_plan);
	// delete[] smooth_spatial;

#ifdef DEBUG
    print_flt_arr(smooth_spatial, 25);
    print_flt_arr(smooth_frequency, 25);
#endif

	for(int i=0; i < height; i+=step)
	{
		/// out_i_start, out_i_end, 控制block数组内需要赋值到arr_out的行数, 保证输出数据没有"黑框"
		int out_i_start, out_i_end;
		if(i == 0){
			out_i_start = 0; out_i_end = size - overlap / 2 - 1;
		}
		else if(i + step - 1 > height - 1){
			out_i_start = overlap / 2; out_i_end = height - 1 - i;
		}
		else{
			out_i_start = overlap / 2; out_i_end = size - overlap / 2 - 1;
		}
		
		int thread_idx = omp_get_thread_num();
		for(int j=0; j < width; j+=step)
		{
			/// out_j_start, out_j_end, 控制block数组内需要赋值到arr_out的列数, 保证输出数据没有"黑框"
			int out_j_start, out_j_end;
			if(j == 0){
				out_j_start = 0; out_j_end = size - overlap / 2 - 1;
			}
			else if(j + step - 1 > width - 1){
				out_j_start = overlap / 2; out_j_end = width - 1 - j;
			}
			else{
				out_j_start = overlap / 2; out_j_end = size - overlap / 2 - 1;
			}

			/// spatial_arr init
			for(int k = 0; k< size*size; k++){
				int block_i = k / size + i;
				int block_j = k % size + j;
				if(block_j > width - 1 || block_i > height - 1){
					/// 说明超界, 需要补零
					spatial_arr[k][0]=0;
					spatial_arr[k][1]=0;
				}
				else{
					spatial_arr[k][0]=arr_in[block_i * width + block_j].real();
					spatial_arr[k][1]=arr_in[block_i * width + block_j].imag();
				}
			}

			///  fft
			fftwf_execute(forward);


			/// abs
			float* block_abs = new float[size*size];
			for(int k=0; k<size*size; k++){
				block_abs[k] = sqrtf(powf(frequency_arr[k][0],2) + powf(frequency_arr[k][1],2));
			}

			/// smooth
			float* block_smooth = new float[size*size];
			// conv_2d(block_abs, size, size, block_smooth, smooth_frequency, 5);
            auto rst = conv_2d(block_abs, size, size, block_smooth, smooth_spatial, 5);

			delete[] block_abs;


			/// block_smooth^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
				frequency_arr[k][0] = block_smooth[k] * alpha * frequency_arr[k][0];
				frequency_arr[k][1] = block_smooth[k] * alpha * frequency_arr[k][1];
			}


			/// ifft
			fftwf_execute(backward);


			/// 赋值
			for(int m = out_i_start; m <= out_i_end; m++){
				for(int n = out_j_start; n <= out_j_end; n++){
                    if(i + m < 0 || i + m > height - 1 || j + n < 0 || j + n > width - 1)
                        continue;
					arr_out[(i+m)*width+(j+n)].real(spatial_arr[m*size+n][0] / size / size);
					arr_out[(i+m)*width+(j+n)].imag(spatial_arr[m*size+n][1] / size / size);
				}
			}


		}
	}


    fftwf_destroy_plan(forward);
    fftwf_destroy_plan(backward);
    fftwf_free(spatial_arr);
    fftwf_free(frequency_arr);

    double spend_sec = spend_time(start_time);
    cout<<"glodstein_single spend_time: "<<spend_sec<<endl;

	return funcrst(true, "filter::goldstein finished.");
}



/// TODO: 写第二版goldstein滤波函数, 并行版, 每一行里创建fftw_plan等fftw需要使用的数据, 该行结束时销毁