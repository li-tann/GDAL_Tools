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

#define EXE_NAME "filter_evaluation"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

float equivalent_number_of_looks(float* arr, float& _mean, float& _std);
float edge_preserving_index(float* arr, float* arr0, int height, int width);
int residue_point_number(float* arr, int height, int width);

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

    if(argc < 5){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [params] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, filtered filepah.\n"
                " argv[2]: input, origin filepath, only epi needed, please print '-' if the method you selected don't need it.\n"
                " argv[3]: method, enl, epi, rpn, ... (single or multi, splited by ',')\n"
                " argv[4]: range, col_start, row_start, width, height (int)\n"
                "          the result will print in cmd and logfile";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    vector<string> evaluation_methods;
	strSplit(string(argv[3]), evaluation_methods, ",");

	if(evaluation_methods.size() < 1){
		return return_msg(-2,"there is no method has chosen.");
	}

    int input_x, input_y, input_width, input_height;
    vector<string> splited_vec;
    strSplit(string(argv[4]), splited_vec, ",");
    if(splited_vec.size() < 4){
		return return_msg(-2,"the size of splited argv[4] is less than 4.");
	}
    input_x = stoi(splited_vec[0]);
    input_y = stoi(splited_vec[1]);
    input_width = stoi(splited_vec[2]);
    input_height = stoi(splited_vec[3]);

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly);
    if(!ds){
        return return_msg(-2, "ds is nullptr.");
    }
    GDALRasterBand* rb = ds->GetRasterBand(1);

    int ds_width = ds->GetRasterXSize();
    int ds_height= ds->GetRasterYSize();
    GDALDataType datatype = rb->GetRasterDataType();

    if(datatype != GDT_Float32){
        return return_msg(-3, "ds.dataype != float.");
    }

    int height = input_height;
    int width  = input_width;
    float* arr = new float[height * width];
    auto cplerr = rb->RasterIO(GF_Read, input_x, input_y, width, height, arr, width, height, datatype, 0, 0);
    if(cplerr != CE_None){
        return return_msg(-3, "input range error to rasterio.");
        GDALClose(ds);  
    }

    GDALClose(ds);  


    bool enl_used = false, epi_used = false, rpn_used = false;
    for(auto method : evaluation_methods)
    {
        return_msg(1, fmt::format("method {}:",method));
        if(method == "enl" && !enl_used){
            enl_used = true;
            float mean, std;
            float enl_val = equivalent_number_of_looks(arr, mean, std);

            return_msg(4, fmt::format("enl value is : {}(mean:{}, std:{})",enl_val, mean, std));
        }
        else if(method == "epi" && !epi_used){
            epi_used = true;
            GDALDataset* ds_ori = (GDALDataset*)GDALOpen(argv[2], GA_ReadOnly);
            if(!ds_ori){
                return_msg(-2, "ds_ori is nullptr.");
                continue;
            }
            GDALRasterBand* rb_ori = ds_ori->GetRasterBand(1);

            int width_ori = ds_ori->GetRasterXSize();
            int height_ori= ds_ori->GetRasterYSize();
            GDALDataType datatype_ori = rb_ori->GetRasterDataType();

            if(width_ori != width || height_ori != height || datatype_ori != datatype){
                GDALClose(ds_ori);
                return_msg(-2, "ds_ori.parm is error.");
                continue;
            }

            float* arr_ori = new float[height * width];
            rb_ori->RasterIO(GF_Read, 0, 0, width, height, arr_ori, width, height, datatype, 0, 0);
            GDALClose(ds_ori);

            float epi_val = edge_preserving_index(arr, arr_ori, height, width);
            delete[] arr_ori;

            return_msg(4, fmt::format("epi value is : {}",epi_val));
        }
        else if(method == "rpn" && !rpn_used){
            rpn_used = true;
            int residue_num = residue_point_number(arr, height, width);

            return_msg(4, fmt::format("residue percent is : {}/{}",residue_num, (height-1)*(width-1)) );
        }
        else{
            return_msg(-2, "unknown method");
        }
    }

    delete[] arr;
    
    return return_msg(1, EXE_NAME " end.");
}

float equivalent_number_of_looks(float* arr, float& _mean, float& _std)
{
    size_t arr_size = dynamic_array_size(arr);
    float mean = 0, std = 0;
    int valid_num = 0;
    for(int i=0; i < arr_size; i++){
        if(isnan(arr[i]))
            continue;

        mean += arr[i];
        std += arr[i] * arr[i];
        valid_num++;
    }
    if(valid_num <= 1)
        return NAN;
    mean /= valid_num;
    std = sqrtf(std / (valid_num-1) - valid_num / (valid_num-1) * mean * mean);

    _mean = mean;
    _std = std;
    return mean * mean / std / std;
}

float edge_preserving_index(float* arr, float* arr0, int height, int width)
{
    auto phase_jump = [](float phase_1, float phase_0){
        float delta = phase_1 - phase_0;
        if(delta >= M_PI)
            delta -= float(2*M_PI);
        else if(delta < -M_PI)
            delta += float(2*M_PI);
        return phase_0 + delta;
    };

    float sum_arr = 0, sum_arr0 = 0;
    for(int i=0; i< height - 1; i++){
        for(int j=0; j< width - 1; j++){
            float arr_phase_origin = arr[i * width + j];
            float arr_phase_right = phase_jump(arr[i * width + j+1], arr_phase_origin);
            float arr_phase_down = phase_jump(arr[(i+1) * width + j], arr_phase_origin);
            
            float arr0_phase_origin = arr0[i * width + j];
            float arr0_phase_right = phase_jump(arr0[i * width + j+1], arr0_phase_origin);
            float arr0_phase_down = phase_jump(arr0[(i+1) * width + j], arr0_phase_origin);

            sum_arr += abs(arr_phase_right - arr_phase_origin) + abs(arr_phase_down - arr_phase_origin);
            sum_arr0 += abs(arr0_phase_right - arr0_phase_origin) + abs(arr0_phase_down - arr0_phase_origin);
        }
    }
    if(sum_arr0 == 0)
        return NAN;
    
    return sum_arr / sum_arr0;
}


int residue_point_number(float* arr, int height, int width)
{
    auto phase_jump = [](float phase_1, float phase_0){
        float delta = phase_1 - phase_0;
        if(delta >= M_PI)
            delta -= float(2*M_PI);
        else if(delta < -M_PI)
            delta += float(2*M_PI);
        return phase_0 + delta;
    };

    auto phase_delta = [](float phase_1, float phase_0){
        float delta = phase_1 - phase_0;
        if(delta >= M_PI)
            delta -= float(2*M_PI);
        else if(delta < -M_PI)
            delta += float(2*M_PI);
        return delta;
    };

    int num_total = 0, num_valid = 0;
    for(int i=0; i< height - 1; i++){
        for(int j=0; j< width - 1; j++){
            float tl = arr[i * width + j];
            float tr = arr[i * width + j+1];
            float dr = arr[(i+1) * width + j+1];
            float dl = arr[(i+1) * width + j];

            float delta_top = phase_delta(tr, tl);
            float delta_right = phase_delta(dr,tr);
            float delta_down = phase_delta(dl,dr);
            float delta_left = phase_delta(tl,dl);
            float delta_sum = delta_top + delta_right + delta_down +delta_left;
            
            num_total++;
            if(abs(delta_sum) < 0.0001){
                num_valid++;
            }
        }
    }

    return num_valid;
}