#ifndef TEMPLATE_SARIMAGE_PROCESS
#define TEMPLATE_SARIMAGE_PROCESS

#include <gdal_priv.h>

#include <vector>
#include <string>
#include <complex>

#include "datatype.h"

using namespace std;

/// @brief 只适用于short int float double
template<typename _Ty>
bool judge_is_nan(_Ty value, double nodatavalue){
    if(value == nodatavalue)
        return true;
    if(is_same<_Ty, short>() || is_same<_Ty, int>()){
        if(value == -32767)
            return true;
    }else if(is_same<_Ty, float>()){
        return _isnanf(float(value));
    }else if(is_same<_Ty, double>()){
        return _isnan(value);
    }
    return false;
};

template<typename _Ty_in, typename _Ty_out>
funcrst arr_multilook(_Ty_in* arr_in, _Ty_out* arr_out, int origin_col, int origin_row, int ml_col, int ml_row)
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
        arr_out = new _Ty_out[mled_row * mled_col];
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
                sum += arr_in[ori_i * origin_col + ori_j];
                num++;
            }
        }
        sum /= num;
        arr_out[index] = _Ty_out(sum);
    }
    return funcrst(true, "arr_multilook success.");
}

template<typename _Ty>
funcrst arr_multilook_cpx(complex<_Ty>* arr_in, complex<_Ty>* arr_out, int origin_col, int origin_row,int ml_col, int ml_row)
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
        arr_out = new complex<_Ty>[mled_row * mled_col];
    }

// #pragma omp parallel for
    for(int index = 0; index < mled_row * mled_col; index++){
        int mled_i = index / mled_col;
        int mled_j = index % mled_col;
        long double sum_real = 0;
        long double sum_imag = 0;
        long double num = 0;
        for(int i=0; i<ml_row; i++){
            int ori_i = i + mled_i * ml_row;
            for(int j=0; j<ml_col; j++) {
                int ori_j = j + mled_j * ml_col;
                /// ori_i ori_j 不可能超出 origin_row 和 origin_col 的界限
                sum_real += arr_in[ori_i * origin_col + ori_j].real();
                sum_imag += arr_in[ori_i * origin_col + ori_j].imag();
                num++;
            }
        }

        sum_real /= num;
        sum_imag /= num;
        arr_out[index].real((_Ty)sum_real);
        arr_out[index].imag((_Ty)sum_imag);
    }
    return funcrst(true, "arr_multilook_cpx success.");
}

enum class sar_image_type{phase, power};

/// @brief arr_in, 只有float和short两种格式
template<typename _Ty_in, typename _Ty_out>
funcrst  cpx_to_real(complex<_Ty_in>* arr_in, _Ty_out* arr_out, sar_image_type type)
{
    size_t arr_length = dynamic_array_size(arr_out);
    if(arr_length <= 0){
        return funcrst(false, "invalid arr_in");
    }

    if(dynamic_array_size(arr_out) != arr_length){
        if(arr_out != nullptr){
            delete[] arr_out;
        }
        arr_out = new _Ty_out[arr_length];
    }



    switch (type)
    {
    case sar_image_type::phase:{
        cout<<"real"<<endl;
        for (size_t i = 0; i < arr_length; i++){
            if (judge_is_nan(arr_in[i].imag(),-32767) || judge_is_nan(arr_in[i].real(),-32767)) {
                arr_out[i] = NAN;
            }else{
                arr_out[i] = atan2(arr_in[i].imag(), arr_in[i].real());
            }
        }
    }break;
    case sar_image_type::power:{
        cout<<"power"<<endl;
        for (size_t i = 0; i < arr_length; i++){
            if (judge_is_nan(arr_in[i].imag(),-32767) || judge_is_nan(arr_in[i].real(),-32767)) {
                arr_out[i] = NAN;
                continue;
            }else{
                arr_out[i] = (_Ty_out)sqrt(pow(arr_in[i].imag(), 2) + pow(arr_in[i].real(), 2));
            }
        }
    }break;
    default:
        return funcrst(false, "unknown sar_image_type");
    }
    return funcrst(true, "cpx_to_real success.");
}

enum class enum_minmax_method{auto_, manual_};

/// @brief 将RasterBand根据需求, 转换为4band(考虑到地理编码时影像存在NAN), 8bit的强度或相位图,
/// @tparam _Ty_in 
/// @tparam _Ty_mem 
/// @param rb_in 
/// @param width 
/// @param height 
/// @param ml 
/// @param image_to 
/// @param minmax_method 
/// @param stretch_percent_or_manual_min 
/// @param manual_max 
/// @param color_map 
/// @param dst_png_filepath 
/// @return 
template<typename _Ty_in, typename _Ty_mem>
funcrst print_sarImage_to_png(
    GDALRasterBand* rb_in, int width, int height, int ml,
    sar_image_type image_to,
    enum_minmax_method minmax_method, double stretch_percent_or_manual_min, double manual_max,
    color_map_v2& color_map,
    const char* dst_png_filepath)
{
    GDALAllRegister();
    funcrst rst(false,"empty");

    GDALDriver* mem_driver = GetGDALDriverManager()->GetDriverByName("MEM");
    GDALDriver* png_driver = GetGDALDriverManager()->GetDriverByName("PNG");

    GDALDataType datatype_in = rb_in->GetRasterDataType();
    GDALDataType datatype_mled_mem;

    double nodatavalue = rb_in->GetNoDataValue();
    cout<<"no_data_value:"<<nodatavalue<<endl;

    switch (datatype_in) /// 支持输入的datatype_in 包括
    {
    case GDT_Int16:
    case GDT_Int32:
    case GDT_Float32:
    case GDT_Float64:
        datatype_mled_mem = datatype_in;
        break;
    case GDT_CFloat32:
        datatype_mled_mem = GDT_Float32;
        break;
    case GDT_CInt16:
        datatype_mled_mem = GDT_Int16;
        break;    
    default:
        return funcrst(false,"unsupported datatype");
        break;
    }
    /// 考虑到需要转成相位, 需要保存小数信息, 但如果是转成强度的话, 因为需要计算平方和, float有可能会出现精度不够的情况, 所以需要保留原本的类型
    if(image_to == sar_image_type::phase)
        datatype_mled_mem = GDT_Float32;
    
    int mled_width = width / ml;
    int mled_height = height / ml;

    cout<<"mled_width:"<<mled_width<<endl;
    cout<<"mled_height:"<<mled_height<<endl;

    GDALDataset* ds_mled_mem = mem_driver->Create("",mled_width, mled_height, 1, datatype_mled_mem, nullptr);
    GDALRasterBand* rb_mled_mem = ds_mled_mem->GetRasterBand(1);
    _Ty_mem* arr_mled_mem = new _Ty_mem[mled_width * mled_height];

    /// 1.multilook, [cpx_to_real], 
    if(datatype_in == GDT_CInt16 || datatype_in == GDT_CFloat32)
    {
        cout<<"datatype_in:"<<datatype_in<<endl;
        /// 如果是复数需要额外转换一个步骤cpx_to_real
        complex<_Ty_in>* arr_in = new complex<_Ty_in>[width * height];
        rb_in->RasterIO(GF_Read, 0, 0, width, height, arr_in, width, height, datatype_in, 0, 0);
        complex<_Ty_in>* arr_mem_temp = new complex<_Ty_in>[mled_width * mled_height];
        rst = arr_multilook_cpx(arr_in, arr_mem_temp, width, height, ml, ml);
        if(!rst){
            return funcrst(false, fmt::format("arr_mulilook_cpx failed, cause :{}.",rst.explain));
        }
        delete[] arr_in;
        rst = cpx_to_real(arr_mem_temp, arr_mled_mem, image_to);
        if(!rst){
            return funcrst(false, fmt::format("cpx_to_real failed, cause :{}.",rst.explain));
        }
        delete[] arr_mem_temp;
        rb_mled_mem->RasterIO(GF_Write, 0,0,mled_width, mled_height, arr_mled_mem, mled_width, mled_height, datatype_mled_mem, 0,0);
    }else
    {
        _Ty_in* arr_in = new _Ty_in[width * height];
        rb_in->RasterIO(GF_Read, 0, 0, width, height, arr_in, width, height, datatype_in, 0, 0);
        arr_multilook(arr_in, arr_mled_mem, width, height, ml, ml);
        delete[] arr_in;
        rb_mled_mem->RasterIO(GF_Write, 0,0,mled_width, mled_height, arr_mled_mem, mled_width, mled_height, datatype_mled_mem, 0,0);
    }
    /// 不知道arr_mem可不可以删除(测试结果没有问题, 可以删除)
    delete[] arr_mled_mem;

    cout<<"multied & to real"<<endl;
    
    /// 完成转换后此时就不在存在复数了, 使用ds_mled_mem, rb_mled_mem, arr_mled_mem这三个变量执行后续处理

    /// 2.get minmax & update colormap
    double min, max;
    if(minmax_method == enum_minmax_method::manual_){
        min = stretch_percent_or_manual_min;
        max = manual_max;
    }else{
        if(stretch_percent_or_manual_min == 0.0){
            double minmax[2];
	        rb_mled_mem->ComputeRasterMinMax(false, minmax);
            min = minmax[0];
            max = minmax[1];
        }
        else{
            cout<<"stretch get minmax, stretch_percent is :"<<stretch_percent_or_manual_min<<endl;
            rst = cal_stretched_minmax(rb_mled_mem, 500,stretch_percent_or_manual_min, min, max);
            if(!rst){
                GDALClose(ds_mled_mem);
                return funcrst(false, fmt::format("cal_stretched_minmax, reason :{}",rst.explain));
            }
        }
    }

    cout<<"min:"<<min<<endl;
    cout<<"max:"<<max<<endl;

    rst = color_map.mapping(float(min), float(max));
    if(!rst){
        GDALClose(ds_mled_mem);
        return funcrst(false, fmt::format("cal_stretched_minmax, reason :{}",rst.explain));
    }
    // color_map.print_colormap();
    // cout<<endl;

    cout<<"get minmax & update colormap"<<endl;

    /// 3. mem -> png_mem(which has 4 bands), print png
    GDALDataset* ds_mem_out = mem_driver->Create("", mled_width, mled_height, 4, GDT_Byte, nullptr);
    if(!ds_mem_out){
        GDALClose(ds_mled_mem);
        return funcrst(false, "ds_mem_out is nullptr.");
    }

    if(is_same<_Ty_mem, short>() || is_same<_Ty_mem, int>())
        nodatavalue = -32768;
    else
        nodatavalue = NAN;
    int invalid_num = 0;
    for(int row=0; row<mled_height; row++)
    {
        unsigned char* arr_mem_out_1 = new unsigned char[mled_width];
        unsigned char* arr_mem_out_2 = new unsigned char[mled_width];
        unsigned char* arr_mem_out_3 = new unsigned char[mled_width];
        unsigned char* arr_mem_out_4 = new unsigned char[mled_width];
        _Ty_mem* arr_mem = new _Ty_mem[mled_width];
        rb_mled_mem->RasterIO(GF_Read, 0,row, mled_width, 1, arr_mem, mled_width, 1, datatype_mled_mem, 0,0);
        for(int col=0; col < mled_width; col++)
        {
            if(judge_is_nan(arr_mem[col],nodatavalue)){
                arr_mem_out_1[col] = 255;
                arr_mem_out_2[col] = 255;
                arr_mem_out_3[col] = 255;
                arr_mem_out_4[col] = 0;
                invalid_num++;
                continue;
            }else{
                rgba color = color_map.mapping_color(float(arr_mem[col]));
                // if(row % 100 == 0 && col == 30){
                //     cout<<arr_mem[col]<<":"<<color.red<<","<<color.green<<","<<color.blue<<","<<color.alpha<<endl;
                // }
                arr_mem_out_1[col] = color.red;
                arr_mem_out_2[col] = color.green;
                arr_mem_out_3[col] = color.blue;
                arr_mem_out_4[col] = color.alpha;
            }
        }
        ds_mem_out->GetRasterBand(1)->RasterIO(GF_Write,0,row, mled_width, 1, arr_mem_out_1, mled_width, 1, GDT_Byte, 0,0);
        ds_mem_out->GetRasterBand(2)->RasterIO(GF_Write,0,row, mled_width, 1, arr_mem_out_2, mled_width, 1, GDT_Byte, 0,0);
        ds_mem_out->GetRasterBand(3)->RasterIO(GF_Write,0,row, mled_width, 1, arr_mem_out_3, mled_width, 1, GDT_Byte, 0,0);
        ds_mem_out->GetRasterBand(4)->RasterIO(GF_Write,0,row, mled_width, 1, arr_mem_out_4, mled_width, 1, GDT_Byte, 0,0);
        delete[] arr_mem_out_1;
        delete[] arr_mem_out_2;
        delete[] arr_mem_out_3;
        delete[] arr_mem_out_4;
        delete[] arr_mem;
    }

    cout<<"invalid_number:"<<invalid_num<<std::endl;
    cout<<"ds_mem_out write finished."<<endl;

    GDALDataset* ds_out = png_driver->CreateCopy(dst_png_filepath,ds_mem_out, TRUE,0,0,0);
    if(!ds_out){
        GDALClose(ds_mled_mem);
        return funcrst(false, "ds_out is nullptr");
    }
  
    GDALClose(ds_mled_mem);
    GDALClose(ds_out);
    GDALClose(ds_mem_out);


    return funcrst(true,"print_sarImage_to_png success.");
}

#endif