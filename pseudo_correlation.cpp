#include "insar_include.h"

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

/*
    argparse::ArgumentParser sub_pseudo_correlation("pseudo");
    sub_pseudo_correlation.add_description("calculate the pseudo correlation data, from complex data.");
    {
        sub_pseudo_correlation.add_argument("input_path")
            .help("input int file path, with fcomplex datatype.");

        sub_pseudo_correlation.add_argument("output_path")
            .help("output pseudo correlation filepath, with float datatype.");
        
        sub_pseudo_correlation.add_argument("win_size")
            .help("the window size used to calculate pseudo correlation, odd integer, default is 5.")
            .scan<'i',int>()
            .default_value("5"); 
    }
*/

int pseudo_correlation(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("input_path");
	std::string output_path = args->get<string>("output_path");
	int size = args->get<int>("win_size");

    if(size % 2 == 0){
        int old_size = size;
        size = size/2*2+1;
        PRINT_LOGGER(logger, warn, fmt::format("win_size input '{}' is not an odd number, which has been replaced with '{}'", old_size, size));
    }

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly);
    if(!ds){
        PRINT_LOGGER(logger, error, "ds is nullptr");
        return -1;
    }
    GDALRasterBand* rb = ds->GetRasterBand(1);

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    GDALDataType datatype = rb->GetRasterDataType();

	if(datatype != GDT_CFloat32){
		PRINT_LOGGER(logger, error, "ds.datatype is not gdt_cfloat32");
        return -1;
	}

    /// got warped phase
	complex<float>* arr = new complex<float>[width * height];
    rb->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);
    GDALClose(ds);
	

    /// cal pseudo correlation
	float* arr_out = new float[width * height];

    // double cpc_spent_sec = class_pseudo_correlation(arr, arr_out, height, width, size);
    // cout<<"cpc_spent_sec: "<<cpc_spent_sec<<"s\n";

    double mpc_spent_sec = my_pseudo_correlation(arr, arr_out, height, width, size);
    cout<<"'my pseudo correlation method' spent_sec: "<<mpc_spent_sec<<"s\n";

    /// write arr_out
	GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(output_path.c_str(), width, height, 1, GDT_Float32, NULL);
    if(!ds_out){
		delete[] arr_out;
        PRINT_LOGGER(logger, error, "ds_out is nullptr.");
        return -2;
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);

	rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, GDT_Float32, 0, 0);

	delete[] arr_out;
	GDALClose(ds_out);

    PRINT_LOGGER(logger, info, "psoudo correlation calculation is finished.");
    return 1;
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