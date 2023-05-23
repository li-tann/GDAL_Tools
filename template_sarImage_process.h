#ifndef TEMPLATE_SARIMAGE_PROCESS
#define TEMPLATE_SARIMAGE_PROCESS

#include <gdal_priv.h>

#include <vector>
#include <string>
#include <complex>

#include "datatype.h"

using namespace std;

template<typename _Ty>
funcrst arr_multilook(_Ty* arr_in, _Ty* arr_out, int origin_col, int origin_row, int ml_col, int ml_row)
{
    if(dynamic_array_size(arr_in) != origin_col * origin_row){
        return funcrst(false, "diff size about arr_in.size and origin_col*origin_row");
    }

    int mled_row = origin_row / ml_row;
    int mled_col = origin_col / ml_col;

    if(dynamic_array_size(arr_out) != mled_row * mled_col){
        if(arr_out != nullptr){
            delete[] arr_out;
        }
        arr_out = new _Ty[mled_row * mled_col];
    }

// #pragma omp parallel for
    for(int index = 0; index < mled_row * mled_col; index++){
        int mled_i = index / mled_col;
        int mled_j = index - mled_i*mled_col;
        long double sum = 0;
        long double num = 0;
        for(int i=0; i<ml_row; i++){
            int ori_i = i + mled_i * ml_row;
            for(int j=0; j<ml_col; j++) {
                int ori_j = j + mled_j * ml_col;
                /// ori_i ori_j 不可能超出 origin_row 和 origin_col 的界限
                sum += arr_in[i * origin_col + ori_j];
                num++;
            }
        }
        sum /= num;
        arr_out[index] = sum;
    }
    return funcrst(true, "arr_multilook success.");
}

template<typename _Ty>
funcrst arr_multilook_cpx(complex<_Ty>* arr_in, complex<_Ty>* arr_out, int ml_col, int ml_row, int origin_col, int origin_row)
{
    if(dynamic_array_size(arr_in) != origin_col * origin_row){
        return funcrst(false, "diff size about arr_in.size and origin_col*origin_row");
    }

    int mled_row = origin_row / ml_row;
    int mled_col = origin_col / ml_col;

    if(dynamic_array_size(arr_out) != mled_row * mled_col){
        if(arr_out != nullptr){
            delete[] arr_out;
        }
        arr_out = new _Ty[mled_row * mled_col];
    }

// #pragma omp parallel for
    for(int index = 0; index < mled_row * mled_col; index++){
        int mled_i = index / mled_col;
        int mled_j = index - mled_i*mled_col;
        long double sum_real = 0;
        long double sum_imag = 0;
        long double num = 0;
        for(int i=0; i<ml_row; i++){
            int ori_i = i + mled_i * ml_row;
            for(int j=0; j<ml_col; j++) {
                int ori_j = j + mled_j * ml_col;
                /// ori_i ori_j 不可能超出 origin_row 和 origin_col 的界限
                sum_real += arr_in[i * origin_col + ori_j].real();
                sum_imag += arr_in[i * origin_col + ori_j].imag();
                num++;
            }
        }
        sum_real /= num;
        sum_imag /= num;
        arr_out[index].real(sum_real);
        arr_out[index].imag(sum_imag);
    }
    return funcrst(true, "arr_multilook_cpx success.");
}

enum class sar_image_type{phase, power};

template<typename _Ty>
funcrst cpx_to_real(complex<_Ty>* arr_in, _Ty* arr_out, sar_image_type type)
{
    size_t arr_length = dynamic_array_size(arr_out);
    if(arr_length <= 0){
        return funcrst(false, "invalid arr_in");
    }

    if(dynamic_array_size(arr_out) != arr_length){
        if(arr_out != nullptr){
            delete[] arr_out;
        }
        arr_out = new _Ty[arr_length];
    }

    switch (type)
    {
    case sar_image_type::phase:{
        for (size_t i = 0; i < arr_length; i++){
            if (isnan(arr_in[i].imag()) || isnan(arr_in[i].real())) {
                arr_out[i] = NAN;
                continue;
            }else{
                arr_out[i] = atan2(arr_in[i].imag(), arr_in[i].real());
            }
        }
    }break;
    case sar_image_type::power:{
        for (size_t i = 0; i < arr_length; i++){
            if (isnan(arr_in[i].imag()) || isnan(arr_in[i].real())) {
                arr_out[i] = NAN;
                continue;
            }else{
                arr_out[i] = sqrt(pow(arr_in[i].imag(), 2) + pow(arr_in[i].real(), 2));
            }
        }
    }
    default:
        return funcrst(false, "unknown sar_image_type");
    }
    return funcrst(true, "cpx_to_real success.");
}

#endif