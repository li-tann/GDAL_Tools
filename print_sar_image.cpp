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
    GDALDataType datatype_in = rb_in->GetRasterDataType();
    if(datatype_in != GDT_UInt16 && datatype_in != GDT_Int16 && 
    datatype_in != GDT_UInt32 && datatype_in != GDT_Int32 &&
    datatype_in != GDT_Float32 && datatype_in != GDT_Float64 &&
    datatype_in != GDT_CFloat32 && datatype_in != GDT_CInt16){
        msg = fmt::format("datatype_in is {}, not a supported datatype.",datatype_in);
        return return_msg(-3,msg);
    }

    /// 1.影像多视处理

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