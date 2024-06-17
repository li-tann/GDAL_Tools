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

#define EXE_NAME "get_image_value_in_line"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

struct _points_info{
    _points_info(int _idx, float _x, float _y):idx(_idx),x(_x),y(_y),value(NAN){}
    int idx;
    float x,y;
    float value;
};

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
                " argv[1]: input, image filepah.\n"
                " argv[2]: input, point of line, start.x,start.y,end.x,end.y,[len] .\n"
                " argv[3]: output, txt, like: idx,point.x,point.y,value";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");


    vector<string> splited_vec;
    strSplit(string(argv[2]),splited_vec,",");

    if(splited_vec.size() < 4){
        return return_msg(-1, fmt::format("number of splited argv[2]({}) less than 4.",splited_vec.size()));
    }

    float start_x = stof(splited_vec[0]);
    float start_y = stof(splited_vec[1]);
    float end_x = stof(splited_vec[2]);
    float end_y = stof(splited_vec[3]);
    int temp_len = 0;
    if(splited_vec.size()>= 5){
        temp_len = stoi(splited_vec[4]);
    }

    int len = (int)sqrt(pow(start_x-end_x,2)+pow(start_y-end_y,2));
    if(temp_len > len)
        len = temp_len;
    
    float delta_x = (end_x - start_x) / len;
    float delta_y = (end_y - start_y) / len;
    vector<_points_info> points_info;
    for(int i=0; i<= len; i++)
        points_info.push_back(_points_info(i, start_x +delta_x * i, start_y + delta_y * i));


    GDALDataset* ds = (GDALDataset*)GDALOpen(argv[1], GA_ReadOnly);
    if(!ds){
        return return_msg(-2, "ds is nullptr.");
    }
    GDALRasterBand* rb = ds->GetRasterBand(1);

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    GDALDataType datatype = rb->GetRasterDataType();

    if(datatype != GDT_Float32){
        return return_msg(-3, "ds.dataype != float.");
    }

    float* arr = new float[4];
    for(auto& info : points_info)
    {
        int tl_x = int(info.x);
        int tl_y = int(info.y);

        if(tl_x < 0 || tl_x > width - 2 || tl_y < 0 || tl_y > height - 2)
            continue;

        rb->RasterIO(GF_Read, tl_x, tl_y, 2, 2, arr, 2, 2, datatype, 0, 0);
        float dx = info.x - tl_x;
        float dy = info.y - tl_y;
        
        info.value = (1 - dy) * (1 - dx) * arr[0] + (1 - dy) * dx * arr[1] + dy * (1 - dx) * arr[2] + dy * dx * arr[3];
    }
    delete[] arr;

    if(string(argv[3]) == "-"){
        for(auto info : points_info){
            cout<<fmt::format("idx[{:<3}], pos({:.3f},{:.3f}), value:{}",info.idx, info.x, info.y, info.value)<<endl;
        }
    }
    else
    {
        ofstream ofs(argv[3]);

        if(!ofs.is_open()){
            return_msg(1, "ofs.open argv[3] failed, result will print in cmd.");

            for(auto info : points_info){
                cout<<fmt::format("idx[{:<3}], pos({:.3f},{:.3f}), value:{}",info.idx, info.x, info.y, info.value)<<endl;
            }
        }
        else{
            for(auto info : points_info){
                ofs<<fmt::format("{},{},{},{}",info.idx, info.x, info.y, info.value)<<endl;
            }
            ofs.close();
        }
    }
    
    
    
    return return_msg(1, EXE_NAME " end.");
}
