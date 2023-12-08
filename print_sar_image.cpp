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

#define EXE_NAME "print_sar_image"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

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
        msg +=  " manual: " EXE_NAME " [sarimage_filepath] [ml] [print_type] [minmax_method] [colormap_filepath] [png_filepath]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input_filepath, supported datatype is short & int & float, scomplex, and fcomplex.\n"
                " argv[2]: multilook.\n"
                " argv[3]: print to (phase or power).\n"
                " argv[4]: minmax method: [auto,0.02(stretch_percentage,0~1)] or [manual,0,1(min & max)].\n"
                " argv[5]: colormap_filepath.\n"
                " argv[6]: print filepath (*.png).\n";
        return return_msg(-1,msg);
    }

    for(int i=0; i<argc; i++){
        cout<<fmt::format("argv[{}]:{},",i,argv[i])<<endl;
    }

    return_msg(0,EXE_NAME " start.");

    /* ------------------------------------ 初始化 ------------------------------------------- */

    /// 多视数
    int multilook = atoi(argv[2]);
    cout<<"multilook: "<<multilook<<endl;

    /// 转phase或power
    std::string argv3 = argv[3];
    sar_image_type image_type = ((argv3=="power") ? sar_image_type::power : sar_image_type::phase);
    cout<<"print_to: "<<((image_type == sar_image_type::power) ? "power" : "phase")<<endl;

    /// 最值设置方式
    vector<string> split_list;
    strSplit(argv[4],split_list,",");
    enum_minmax_method minmax_method = (split_list[0] == "auto") ? enum_minmax_method::auto_ : enum_minmax_method::manual_;
    double stretch_percent;
    double manual_min, manual_max;
    double stretch_percent_or_manual_min;
    cout<<"get_minmax_method: ";
    switch (minmax_method)
    {
    case enum_minmax_method::auto_:{
        if(split_list.size()<2){
            msg = "minmax method, less stretch percent.";
            return return_msg(-2,msg);
        }
        stretch_percent = stod(split_list[1]);
        stretch_percent_or_manual_min = stretch_percent;
        cout<<"auto, stretch_percent is "<<stretch_percent_or_manual_min<<endl;
    }break;
    case enum_minmax_method::manual_:{
        if(split_list.size()<3){
            msg = "minmax method, less min or max.";
            return return_msg(-2,msg);
        }
        manual_min = stod(split_list[1]);
        manual_max = stod(split_list[2]);
        stretch_percent_or_manual_min = manual_min;
        cout<<"manual, min&max is "<<stretch_percent_or_manual_min<<","<<manual_max<<endl;
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
    if(datatype_in != GDT_Int16 && datatype_in != GDT_Int32 &&
    datatype_in != GDT_Float32 && datatype_in != GDT_Float64 &&
    datatype_in != GDT_CFloat32 && datatype_in != GDT_CInt16){
        GDALClose(ds_in);
        msg = fmt::format("datatype_in is {}, not a supported datatype.",datatype_in);
        return return_msg(-3,msg);
    }

    cout<<"GDALDatatype:"<<datatype_in<<endl;
    return_msg(1,"start print_sarImage_to_png");

    switch (datatype_in)
    {
    case GDT_Int16:{
        if(image_type == sar_image_type::phase)
            rst = print_sarImage_to_png<short, short>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
        else/// power
            rst = print_sarImage_to_png<short, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
    }break;
    case GDT_Int32:{
        if(image_type == sar_image_type::phase)
            rst = print_sarImage_to_png<int, int>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
        else/// power
            rst = print_sarImage_to_png<int, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
    }break;
    case GDT_Float32:{
        if(image_type == sar_image_type::phase)
            rst = print_sarImage_to_png<float, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
        else/// power
            rst = print_sarImage_to_png<float, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
    }break;
    case GDT_Float64:{
        if(image_type == sar_image_type::phase)
            rst = print_sarImage_to_png<double, double>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
        else/// power
            rst = print_sarImage_to_png<double, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
    }break;
    case GDT_CInt16:{
        if(image_type == sar_image_type::phase)
            rst = print_sarImage_to_png<short, short>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
        else/// power
            rst = print_sarImage_to_png<short, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
    }break;
    case GDT_CFloat32:{
        cout<<"print_sarImage_to_png, cfloat32"<<endl;
        if(image_type == sar_image_type::phase)
            rst = print_sarImage_to_png<float, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
        else/// power
            rst = print_sarImage_to_png<float, float>(rb_in, width, height, multilook, image_type, minmax_method, stretch_percent_or_manual_min, manual_max, color_map, argv[6]);
    }break;
    
    default:
        break;
    }
    
    if(!rst){
        return_msg(2, rst.explain);
    }
    

    return return_msg(1, EXE_NAME " end.");
}