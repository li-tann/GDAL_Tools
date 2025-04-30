#include "insar_include.h"

funcrst goldstein(std::complex<float>* arr_in, int height, int width, float alpha, std::complex<float>* arr_out);
funcrst goldstein_single(std::complex<float>* arr_in, int height, int width, float alpha, std::complex<float>* arr_out);

/// @note 创建sinc窗口, 应该只适用于偶数
float* sinc_window(int height, int width)
{
    float* dst = new float[height*width];
    float* arr_x = new float[width];
    float* arr_y = new float[height];

    for(int i=0; i<width; i++){
        float x = (i - width/2 + 0.5)/(width/2);
        arr_x[i] = sin(M_PI*x) / (M_PI*x);
        if(isnan(arr_x[i])) arr_x[i] = 1;
    }

    for(int i=0; i<height; i++){
        float y = (i - height/2 + 0.5)/(height/2);
        arr_y[i] = sin(M_PI*y) / (M_PI*y);
        if(isnan(arr_y[i])) arr_y[i] = 1;
    }

    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            dst[i*width+j] = arr_x[j] * arr_y[i]; 
        }
    }
    delete[] arr_x;
    delete[] arr_y;

    return dst;
}

/// @note 同样的 应该只适用于偶数
void arr_2d_shift(float* arr, int height, int width)
{
    float tmp;
    /// 水平对调
    for(int i=0; i<height; i++){
        for(int j=0; j<width/2; j++){
            tmp = arr[i*width+j];
            arr[i*width+j] = arr[i*width+j+width/2];
            arr[i*width+j+width/2] = tmp;
        }
    }

    /// 垂直对调
    for(int j=0; j<width; j++){
        for(int i=0; i<height/2; i++){
            tmp = arr[i*width+j];
            arr[i*width+j] = arr[(i+height/2)*width+j];
            arr[(i+height/2)*width+j] = tmp;
        }
    }
}

/*
	argparse::ArgumentParser sub_goldstein("goldstein");
    sub_goldstein.add_description("determine relationship between point and shapefile.");
    {
        sub_goldstein.add_argument("input_path")
            .help("input file path, support float and fcomplex.");
        
        sub_goldstein.add_argument("output_path")
            .help("filterd output file path, with same datatype with input_path.");

        sub_goldstein.add_argument("-a","--alpha")
            .help("the power of window, within range [0,1], , default is 0.5. The higher the power, the more pronounced the filtering effect.")
            .scan<'g',double>()
            .default_value("0.5");   
    }
*/

int filter_goldstein(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
	std::string input_path  = args->get<string>("input_path");
	std::string output_path = args->get<string>("output_path");
	double alpha = args->get<double>("--alpha");

	if(alpha < 0 || alpha >1){
		PRINT_LOGGER(logger, warn, fmt::format("alpha input is a invalid data ({}) which has been replaced by default ({})",alpha, 0.5));
		alpha = 0.5;
	}

	GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly);
    if(!ds){
        PRINT_LOGGER(logger, error, "ds is nullptr");
		return -1;
    }
	int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    GDALRasterBand* rb = ds->GetRasterBand(1);
    GDALDataType datatype = rb->GetRasterDataType();

	if(datatype != GDT_CFloat32 && datatype != GDT_Float32){
		PRINT_LOGGER(logger, error, "datatype is diff with float or fcomplex.");
		return -2;
	}

	std::complex<float>* arr = new std::complex<float>[width * height];
	if(datatype == GDT_CFloat32){
		rb->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);
	}
	else{
		/// datatype == GDT_Float32
		float* arr_flt = new float[width * height];	/// 直接全申请, 更省事, 并且如果这里都申请不下来的话, 后面的arr_out肯定更不行
		rb->RasterIO(GF_Read, 0, 0, width, height, arr_flt, width, height, datatype, 0, 0);
		for(int i=0; i<width * height;i++){
			arr[i].real(cos(arr_flt[i]));
			arr[i].imag(sin(arr_flt[i]));
		}
		delete[] arr_flt;
	}
	std::complex<float>* arr_out = new std::complex<float>[width * height];
	
	PRINT_LOGGER(logger, info, "Preparation completed, start filtering with goldstein.");

	funcrst rst = goldstein(arr, height, width, alpha, arr_out);
	if(!rst){
		PRINT_LOGGER(logger, error, fmt::format("goldstein failed. ({})", rst.explain));
		return -3;
	}

    delete[] arr;
	GDALClose(ds);

	GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(output_path.c_str(), width, height, 1, datatype, NULL);
    if(!ds_out){
		delete[] arr_out;
        PRINT_LOGGER(logger, error, "ds_out is nullptr.");
		return -3;
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
	if(datatype == GDT_CFloat32){
		rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, datatype, 0, 0);
	}
	else{
		/// datatype == GDT_Float32
		float* arr_flt = new float[width * height];	/// 直接全申请, 更省事, 并且如果这里都申请不下来的话, 后面的arr_out肯定更不行
		for(int i=0; i<width * height;i++){
			arr_flt[i] = atan2(arr[i].imag(), arr[i].real());
		}
		rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_flt, width, height, datatype, 0, 0);
		delete[] arr_flt;
	}
	delete[] arr_out;
	GDALClose(ds_out);

	PRINT_LOGGER(logger, info, "filter_goldstein finished.");
	return 1;
}

funcrst conv_2d(float* arr_in, int width, int height, float* arr_out, float* kernel, int size)
{
	if(arr_in == nullptr)
		return funcrst(false, "filter::conv_2d, arr_in is nullptr.");
	
	// if(dynamic_array_size(arr_in) != width * height)
	// 	return funcrst(false, fmt::format("filter::conv_2d, arr_in.size({}) is diff with width*height({}).",dynamic_array_size(arr_in),width * height));
	
	if(kernel == nullptr)
		return funcrst(false, "filter::conv_2d, kernel is nullptr.");

	// if(dynamic_array_size(kernel) != size * size)
	// 	return funcrst(false, fmt::format("filter::conv_2d, kernel.size({}) is diff with size^2({}).",dynamic_array_size(kernel),size*size));

	float* kernel_overturn = new float[size*size];
	for(int i=0; i<size*size; i++)
		kernel_overturn[i] = kernel[size*size-1-i];


	if(arr_out){
		delete[] arr_out;
		arr_out = nullptr;
	}
	arr_out = new float[height * width];

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

	if(arr_out){
		delete[] arr_out;
		arr_out = nullptr;
	}
	arr_out = new std::complex<float>[height * width];

	int max_threads = omp_get_max_threads();

	fftwf_complex** spatial_arrs =  new fftwf_complex*[max_threads];
	fftwf_complex** frequency_arrs =  new fftwf_complex*[max_threads];
	fftwf_plan* forward_plans = new fftwf_plan[max_threads];
	fftwf_plan* backward_plans = new fftwf_plan[max_threads];
	for(int i=0; i < max_threads; i++){
		spatial_arrs[i] = fftwf_alloc_complex(size*size);
		frequency_arrs[i] = fftwf_alloc_complex(size*size);
		forward_plans[i] = fftwf_plan_dft_2d(size, size, spatial_arrs[i], frequency_arrs[i], FFTW_FORWARD, FFTW_ESTIMATE);
		backward_plans[i] = fftwf_plan_dft_2d(size, size, frequency_arrs[i], spatial_arrs[i], FFTW_BACKWARD, FFTW_ESTIMATE);
	}
	
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
					spatial_arrs[thread_idx][k][0]=0;
					spatial_arrs[thread_idx][k][1]=0;
				}
				else{
					spatial_arrs[thread_idx][k][0]=arr_in[block_i * width + block_j].real();
					spatial_arrs[thread_idx][k][1]=arr_in[block_i * width + block_j].imag();
				}
			}

			///  fft
			fftwf_execute(forward_plans[thread_idx]);


			/// abs
			float* block_abs = new float[size*size];
			for(int k=0; k<size*size; k++){
				frequency_arrs[thread_idx][k][0] /= size*size;
                frequency_arrs[thread_idx][k][1] /= size*size;
				block_abs[k] = sqrtf(powf(frequency_arrs[thread_idx][k][0],2) + powf(frequency_arrs[thread_idx][k][1],2));
			}
#if true
			/// @note create sinc window
            float* sinc = sinc_window(size, size);
            /// @note sinc window shift
			arr_2d_shift(sinc, size, size);
            /// @note filtered = sinc_win * freq_amp 
			float* filtered_arrs = new float[size*size];
            for(int i=0; i<size*size; i++){
                filtered_arrs[i] = sinc[i]*block_abs[i];
            }

			delete[] block_abs;
			delete[] sinc;


			/// filtered^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
                float scale = pow(filtered_arrs[k], alpha);
				frequency_arrs[thread_idx][k][0] = scale * frequency_arrs[thread_idx][k][0];
				frequency_arrs[thread_idx][k][1] = scale * frequency_arrs[thread_idx][k][1];
			}
			delete[] filtered_arrs;
#else			
			/// smooth
			float* smooth_spatial = new float[25];
			for(int k = 0; k<25; k++)
				smooth_spatial[k] = 0.04;
			float* block_smooth = new float[size*size];
			// conv_2d(block_abs, size, size, block_smooth, smooth_frequency, 5);
            auto rst = conv_2d(block_abs, size, size, block_smooth, smooth_spatial, 5);

			delete[] block_abs;
			delete[] smooth_spatial;


			/// block_smooth^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
				frequency_arrs[thread_idx][k][0] = pow(block_smooth[k],alpha) * frequency_arrs[thread_idx][k][0];
				frequency_arrs[thread_idx][k][1] = pow(block_smooth[k],alpha) * frequency_arrs[thread_idx][k][1];
			}

			delete[] block_smooth;
#endif

			/// ifft
			fftwf_execute(backward_plans[thread_idx]);


			/// 赋值
			for(int m = out_i_start; m <= out_i_end; m++){
				for(int n = out_j_start; n <= out_j_end; n++){
                    if(i + m < 0 || i + m > height - 1 || j + n < 0 || j + n > width - 1)
                        continue;
					arr_out[(i+m)*width+(j+n)].real(spatial_arrs[thread_idx][m*size+n][0] / size / size);
					arr_out[(i+m)*width+(j+n)].imag(spatial_arrs[thread_idx][m*size+n][1] / size / size);
				}
			}


		}
		
	}

	for(int i=0; i< max_threads; i++){
		fftwf_destroy_plan(forward_plans[i]);
		fftwf_destroy_plan(backward_plans[i]);
		fftwf_free(spatial_arrs[i]);
		fftwf_free(frequency_arrs[i]);
	}
	

    double spend_sec = spend_time(start_time);
    cout<<"glodstein_single spend_time: "<<spend_sec<<endl;

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

	if(arr_out){
		delete[] arr_out;
		arr_out = nullptr;
	}
	arr_out = new std::complex<float>[height * width];

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
				frequency_arr[k][0] /= size*size;
                frequency_arr[k][1] /= size*size;
				block_abs[k] = sqrtf(powf(frequency_arr[k][0],2) + powf(frequency_arr[k][1],2));
			}

#if true
			/// @note create sinc window
            float* sinc = sinc_window(size, size);
            /// @note sinc window shift
			arr_2d_shift(sinc, size, size);
            /// @note filtered = sinc_win * freq_amp 
			float* filtered_arrs = new float[size*size];
            for(int i=0; i<size*size; i++){
                filtered_arrs[i] = sinc[i]*block_abs[i];
            }

			delete[] block_abs;
			delete[] sinc;

			/// filtered^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
                float scale = pow(filtered_arrs[k], alpha);
				frequency_arr[k][0] = scale * frequency_arr[k][0];
				frequency_arr[k][1] = scale * frequency_arr[k][1];
			}
			delete[] filtered_arrs;
#else			
			/// smooth
			float* block_smooth = new float[size*size];
			// conv_2d(block_abs, size, size, block_smooth, smooth_frequency, 5);
            auto rst = conv_2d(block_abs, size, size, block_smooth, smooth_spatial, 5);

			delete[] block_abs;


			/// block_smooth^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
				// frequency_arr[k][0] = block_smooth[k] * alpha * frequency_arr[k][0];
				// frequency_arr[k][1] = block_smooth[k] * alpha * frequency_arr[k][1];
				frequency_arr[k][0] = pow(block_smooth[k],alpha) * frequency_arr[k][0];
				frequency_arr[k][1] = pow(block_smooth[k],alpha) * frequency_arr[k][1];
			}

			delete[] block_smooth;
#endif

			


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







funcrst goldstein_single_float(float* arr_in, int height, int width, float alpha, float* arr_out);

int filter_goldstein_float(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
	std::string input_path  = args->get<string>("input_path");
	std::string output_path = args->get<string>("output_path");
	double alpha = args->get<double>("--alpha");

	if(alpha < 0 || alpha >1){
		PRINT_LOGGER(logger, warn, fmt::format("alpha input is a invalid data ({}) which has been replaced by default ({})",alpha, 0.5));
		alpha = 0.5;
	}

	GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly);
    if(!ds){
        PRINT_LOGGER(logger, error, "filter_goldstein_float failed, ds is nullptr");
		return -1;
    }
	int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    GDALRasterBand* rb = ds->GetRasterBand(1);
    GDALDataType datatype = rb->GetRasterDataType();

	if(datatype != GDT_Float32){
		PRINT_LOGGER(logger, error, "filter_goldstein_float failed, datatype is diff with float.");
		return -2;
	}

	float* arr = new float[width * height];
	rb->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);


	float* arr_out = new float[width * height];
	
	PRINT_LOGGER(logger, info, "Preparation completed, start filtering with goldstein.");

	funcrst rst = goldstein_single_float(arr, height, width, alpha, arr_out);
	if(!rst){
		PRINT_LOGGER(logger, error, fmt::format("filter_goldstein_float failed. ({})", rst.explain));
		return -3;
	}

    delete[] arr;
	GDALClose(ds);

	GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(output_path.c_str(), width, height, 1, datatype, NULL);
    if(!ds_out){
		delete[] arr_out;
        PRINT_LOGGER(logger, error, "filter_goldstein_float failed, ds_out is nullptr.");
		return -3;
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
	rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, datatype, 0, 0);

	delete[] arr_out;
	GDALClose(ds_out);

	PRINT_LOGGER(logger, info, "filter_goldstein_float finished.");
	return 1;
}

funcrst goldstein_single_float(float* arr_in, int height, int width, float alpha, float* arr_out)
{
    auto start_time = std::chrono::system_clock::now();

	int size = 32;
	int overlap = 24;
	int step = size - overlap;

	/// 避免多线程时多线程同时调用一个plan出现异常
    float* spatial_arr =  new float[size*size];
	fftwf_complex* frequency_arr = fftwf_alloc_complex(size*size);
	fftwf_plan forward  = fftwf_plan_dft_r2c_2d(size, size, spatial_arr, frequency_arr, FFTW_ESTIMATE);
	fftwf_plan backward = fftwf_plan_dft_c2r_2d(size, size, frequency_arr, spatial_arr, FFTW_ESTIMATE);

	if(arr_out){
		delete[] arr_out;
		arr_out = nullptr;
	}
	arr_out = new float[height * width];

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
					spatial_arr[k]=0;
				}
				else{
					spatial_arr[k]=arr_in[block_i * width + block_j];
				}
			}

			///  fft
			fftwf_execute(forward);


			/// abs
			float* block_abs = new float[size*size];
			for(int k=0; k<size*size; k++){
				frequency_arr[k][0] /= size*size;
                frequency_arr[k][1] /= size*size;
				block_abs[k] = sqrtf(powf(frequency_arr[k][0],2) + powf(frequency_arr[k][1],2));
			}

#if true
			/// @note create sinc window
            float* sinc = sinc_window(size, size);
            /// @note sinc window shift
			arr_2d_shift(sinc, size, size);
            /// @note filtered = sinc_win * freq_amp 
			float* filtered_arrs = new float[size*size];
            for(int i=0; i<size*size; i++){
                filtered_arrs[i] = sinc[i]*block_abs[i];
            }

			delete[] block_abs;
			delete[] sinc;

			/// filtered^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
                float scale = pow(filtered_arrs[k], alpha);
				frequency_arr[k][0] = scale * frequency_arr[k][0];
				frequency_arr[k][1] = scale * frequency_arr[k][1];
			}
			delete[] filtered_arrs;
#else			
			/// smooth
			float* block_smooth = new float[size*size];
			// conv_2d(block_abs, size, size, block_smooth, smooth_frequency, 5);
            auto rst = conv_2d(block_abs, size, size, block_smooth, smooth_spatial, 5);

			delete[] block_abs;


			/// block_smooth^alpha * spatial_arrs -> frequency_arrs
			for(int k=0; k< size*size; k++){
				// frequency_arr[k][0] = block_smooth[k] * alpha * frequency_arr[k][0];
				// frequency_arr[k][1] = block_smooth[k] * alpha * frequency_arr[k][1];
				frequency_arr[k][0] = pow(block_smooth[k],alpha) * frequency_arr[k][0];
				frequency_arr[k][1] = pow(block_smooth[k],alpha) * frequency_arr[k][1];
			}

			delete[] block_smooth;
#endif


			/// ifft
			fftwf_execute(backward);


			/// 赋值
			for(int m = out_i_start; m <= out_i_end; m++){
				for(int n = out_j_start; n <= out_j_end; n++){
                    if(i + m < 0 || i + m > height - 1 || j + n < 0 || j + n > width - 1)
                        continue;
					arr_out[(i+m)*width+(j+n)] = (spatial_arr[m*size+n] / size / size);
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
