#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <fstream>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"
#include "template_sarImage_process.h"

#define EXE_NAME "_print_sar_image"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

void strSplit(std::string input, std::vector<std::string>& output, std::string split, bool clearVector = true);
double spend_time(decltype (std::chrono::system_clock::now()) start);

enum class enum_minmax_method{auto_, manual_};

int main(int argc, char* argv[])
{

    auto start = chrono::system_clock::now();

    GDALAllRegister();
    funcrst rst;
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    ///2种方式: 1.强度图, 2.相位图
    /// 强度图与相位图的共同参数 1.影像地址, 2.多视倍数, 3.打印方法, 4.最值设置( auto,2 / manual,1,0 ), 5.颜色表, 6.输出地址
    if(argc < 7){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input_filepath, supported datatype is ushort& short & int & uint & float, scomplex, and fcomplex.\n"
                " argv[2]: multilook.\n"
                " argv[3]: print to (phase or power).\n"
                " argv[4]: minmax method (auto,[percent(0.02)] or manual,[min(0)],[max(1)]).\n"
                " argv[5]: colormap_filepath.\n"
                " argv[6]: print filepath (*.png).\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    /// 多视数
    int multilook = atoi(argv[2]);

    /// 转phase或power
    sar_image_type image_type = argv[3]=="power" ? sar_image_type::power : sar_image_type::phase;

    /// 最值设置方式
    vector<string> split_list;
    strSplit(argv[4],split_list,",");
    enum_minmax_method minmax_method = split_list[0] == "auto" ? enum_minmax_method::auto_ : enum_minmax_method::manual_;
    double stretch_percent;
    double manual_min, manual_max;
    switch (minmax_method)
    {
    case enum_minmax_method::auto_:{
        if(split_list.size()<2){
            msg = "minmax method, less stretch percent.";
            return_msg(-2,msg);
        }
        stretch_percent = stod(split_list[1]);
    }break;
    case enum_minmax_method::manual_:{
        if(split_list.size()<3){
            msg = "minmax method, less min or max.";
            return_msg(-2,msg);
        }
        manual_min = stod(split_list[1]);
        manual_max = stod(split_list[2]);
    }break;
    default:
        break;
    }

    /// 颜色表
    color_map_v2 color_map(argv[5]);
    if(!color_map.is_opened()){
        msg = "color_map open failed.";
        return return_msg(-3,msg);
    }


    GDALAllRegister();

    /// 判断影像格式, 不接受非常规数据格式

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(argv[1],GA_ReadOnly);
    if(!ds_in){
        msg = "ds_in is nullptr";
        return return_msg(-2,msg);
    }
    GDALRasterBand* rb_in = ds_in->GetRasterBand(1);

    int width = ds_in->GetRasterXSize();
    int height = ds_in->GetRasterYSize();

    int mled_row = height / multilook;
    int mled_col = width / multilook;

    GDALDataType datatype_in = rb_in->GetRasterDataType();
    if(datatype_in != GDT_UInt16 && datatype_in != GDT_Int16 && 
    datatype_in != GDT_UInt32 && datatype_in != GDT_Int32 &&
    datatype_in != GDT_Float32 && datatype_in != GDT_Float64 &&
    datatype_in != GDT_CFloat32 && datatype_in != GDT_CInt16){
        GDALClose(ds_in);
        msg = fmt::format("datatype_in is {}, not a supported datatype.",datatype_in);
        return return_msg(-3,msg);
    }


    GDALDataType datatype_mem = datatype_in;
    if(datatype_mem == GDT_CFloat32) datatype_mem = GDT_Float32;
    if(datatype_mem == GDT_CInt16) datatype_mem = GDT_Int16;

    /// 1.影像多视处理, 并转换为实数影像,并生成一个MEM的影像
    GDALDriver* mem_driver = GetGDALDriverManager()->GetDriverByName("MEM");
    GDALDataset* ds_mem = mem_driver->Create("", mled_row, mled_col, 1, datatype_mem, nullptr);
    GDALRasterBand* rb_mem = ds_mem->GetRasterBand(1);
    switch (datatype_in)
    {
    case GDT_UInt16:{
        /// 多视
        unsigned short* arr = new unsigned short[width * height];
        rb_in->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype_in, 0, 0);
        unsigned short* arr_multied = nullptr;
        arr_multilook(arr, arr_multied, width, height, multilook, multilook);
        delete[] arr;
        rb_mem->RasterIO(GF_Write, 0,0,mled_col, mled_row, arr_multied, mled_col, mled_row, datatype_mem, 0,0);

        /// 最值
        double min, max;
        if(minmax_method == enum_minmax_method::manual_){
            min = manual_min;
            max = manual_max;
        }else{
            rst = cal_stretched_minmax(rb_mem, 255,stretch_percent, min, max);
            if(!rst){
                GDALClose(ds_in);
                msg = fmt::format("cal_stretched_minmax, reason :{}",rst.explain);
                return return_msg(-4,msg);
            }
        }
    }break;
    
    default:
        break;
    }

    /// 想要方便写模板函数或者宏的话, 操作单位最好是RasterBand

{
     /// 如果是float 格式的话

    datatype_in = GDT_Float32;
    datatype_mem = GDT_Float32;
    GDALDataset* ds_mem = mem_driver->Create("", mled_row, mled_col, 1, datatype_mem, nullptr);
    GDALRasterBand* rb_mem = ds_mem->GetRasterBand(1);
    float* arr_mem = new float[mled_row * mled_col];

    /// 1.multilook, create mem

    float* arr_in = new float[width * height];
    rb_in->RasterIO(GF_Read, 0, 0, width, height, arr_in, width, height, datatype_in, 0, 0);
    arr_multilook(arr_in, arr_mem, width, height, multilook, multilook);
    delete[] arr_in;
    rb_mem->RasterIO(GF_Write, 0,0,mled_col, mled_row, arr_mem, mled_col, mled_row, datatype_mem, 0,0);
    /// 不知道arr_mem可不可以删除
    delete[] arr_mem;

    /// 2.get min&max

    if(1){}

    /// 3.create colormap, and update mem

    /// 4.print mem
}

{
    /// 如果是float complex 格式的话

    datatype_in = GDT_CFloat32;
    datatype_mem = GDT_Float32;
    GDALDataset* ds_mem = mem_driver->Create("", mled_row, mled_col, 1, datatype_mem, nullptr);
    GDALRasterBand* rb_mem = ds_mem->GetRasterBand(1);
    float* arr_mem = new float[mled_row * mled_col];

    /// 1.multilook, [cpx_tO_real], create mem

    complex<float>* arr_in = new complex<float>[width * height];
    rb_in->RasterIO(GF_Read, 0, 0, width, height, arr_in, width, height, datatype_in, 0, 0);
    complex<float>* arr_mem_temp = new complex<float>[mled_row * mled_col];
    arr_multilook_cpx(arr_in, arr_mem_temp, width, height, multilook, multilook);
    delete[] arr_in;
    cpx_to_real(arr_mem_temp, arr_mem, image_type);
    delete[] arr_mem_temp;
    rb_mem->RasterIO(GF_Write, 0,0,mled_col, mled_row, arr_mem, mled_col, mled_row, datatype_mem, 0,0);
    /// 不知道arr_mem可不可以删除
    delete[] arr_mem;

    /// 2.get min&max

    double min, max;
    if(minmax_method == enum_minmax_method::manual_){
        min = manual_min;
        max = manual_max;
    }else{
        rst = cal_stretched_minmax(rb_mem, 255,stretch_percent, min, max);
        if(!rst){
            GDALClose(ds_in);
            GDALClose(ds_mem);
            msg = fmt::format("cal_stretched_minmax, reason :{}",rst.explain);
            return return_msg(-4,msg);
        }
    }
 
    /// 3.create colormap

    rst = color_map.mapping(min, max);

    /// 4.print mem

    GDALDriver* png_driver = GetGDALDriverManager()->GetDriverByName("PNG");
    GDALDataset* ds_mem_out = mem_driver->Create("", mled_row, mled_col, 4, GDT_Byte, nullptr);
    for(int row=0; row<mled_row; row++)
    {
        char* arr_mem_out_1 = new char[mled_col];
        char* arr_mem_out_2 = new char[mled_col];
        char* arr_mem_out_3 = new char[mled_col];
        char* arr_mem_out_4 = new char[mled_col];
        float* arr_mem = new float[mled_col];
        rb_mem->RasterIO(GF_Read, 0,row, mled_col, 1, arr_mem, mled_col, 1, datatype_mem, 0,0);
        for(int col=0; col < mled_col; col++)
        {
            if(isnan(arr_mem[col])){
                arr_mem_out_1[col] = 0;
                arr_mem_out_2[col] = 0;
                arr_mem_out_3[col] = 0;
                arr_mem_out_4[col] = 255;
                continue;
            }else{
                rgba color = color_map.mapping_color(arr_mem[col]);
                arr_mem_out_1[col] = color.red;
                arr_mem_out_2[col] = color.green;
                arr_mem_out_3[col] = color.blue;
                arr_mem_out_4[col] = color.alpha;
            }
        }
        ds_mem_out->GetRasterBand(1)->RasterIO(GF_Write,0,row, mled_col, 1, arr_mem_out_1, mled_col, 1, datatype_mem, 0,0);
        ds_mem_out->GetRasterBand(2)->RasterIO(GF_Write,0,row, mled_col, 1, arr_mem_out_2, mled_col, 1, datatype_mem, 0,0);
        ds_mem_out->GetRasterBand(3)->RasterIO(GF_Write,0,row, mled_col, 1, arr_mem_out_3, mled_col, 1, datatype_mem, 0,0);
        ds_mem_out->GetRasterBand(4)->RasterIO(GF_Write,0,row, mled_col, 1, arr_mem_out_4, mled_col, 1, datatype_mem, 0,0);
        delete[] arr_mem_out_1;
        delete[] arr_mem_out_2;
        delete[] arr_mem_out_3;
        delete[] arr_mem_out_4;
        delete[] arr_mem;
    }



    GDALDataset* ds_out = png_driver->CreateCopy(argv[6],ds_mem_out, TRUE,0,0,0);

    /// 填写png的数组

}







    /// 2.获取影像的最值, (现有的基于rasterband的方法, 需要先创建一个空地址的dataset(但因为这个dataset最后生成png时也要使用, 所以不会浪费时间))

    /// 3.创建颜色表, 并且根据最值更新颜色表的投影关系，依据投影关系再次更新上面那个dataset

    /// 4.dataset打印输出


    /// float想彩色投影的一个示例
    // for (size_t i = 0; i < size_t(src_img_width) * size_t(src_img_height); i++)
    // {
    //     if (isnan(src_data[i])) {
    //         qimage_data[i] = qRgba(0, 0, 0, 255);
    //         continue;
    //     }
    //     float val = src_data[i];
    //     rgba color = ct.mapping_color(src_data[i]);
    //     qimage_data[i] = qRgba(color.red, color.green, color.blue, color.alpha);
    // }



    

    return_msg(2, msg);

    return return_msg(1, EXE_NAME " end.");
}


void strSplit(std::string input, std::vector<std::string>& output, std::string split, bool clearVector)
{
    if(clearVector)
        output.clear();
    std::string::size_type pos1, pos2;
    pos1 = input.find_first_not_of(split);
    pos2 = input.find(split,pos1);

    if (pos1 == std::string::npos) {
        return;
    }
    if (pos2 == std::string::npos) {
        output.push_back(input.substr(pos1));
        return;
    }
    output.push_back(input.substr(pos1, pos2 - pos1));
    strSplit(input.substr(pos2 + 1), output, split,false);
    
}

double spend_time(decltype (std::chrono::system_clock::now()) start)
{
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double second = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    return second;
}

bool print_to_power();

bool print_to_phase();