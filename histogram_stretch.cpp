#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "template_stretch.h"

#define EXE_NAME "histogram_stretch"
#define HISTOGRAM_SIZE 256

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

/// 未测试

int main(int argc, char* argv[])
{
    GDALAllRegister();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input imgpath,\n"
                " argv[2]: stretch rate(double), within (0,0.5], normal number is 0.02 or 0.05,\n"
                " argv[3]: output imgpath,\n";
        return return_msg(-1,msg);
    }

    if(string(argv[1])!= string(argv[3])){
        fs::path src(argv[1]);
        fs::path dst(argv[2]);
        if(fs::exists(dst)){
            msg = "argv[3] is existed, we will remove and copy argv[1] to argv[3].";
            return_msg(0,msg);
            fs::remove(dst);
        }
        fs::copy(src,dst);
    }

    double stretch_rate = atof(argv[2]);
    
    return_msg(0,EXE_NAME " start.");

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(argv[3],GA_Update));
    if(ds == nullptr)
        return return_msg(-2,"ds is nullptr. argv[3] is no-existed(when argv[1]==argv[3]), or cann't be create?");

    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();
    GDALDataType datatype = ds->GetRasterBand(1)->GetRasterDataType();
    if(datatype > GDT_Float64 /*cshort cint cfloat cdouble*/ || datatype == 0 /*unknown*/){
        return return_msg(-3,"unsupported datatype, unsupport list: complex");
    }

    for(int b = 1; b <= bands; b++)
    {
        GDALRasterBand* rb = ds->GetRasterBand(b);
        tuple_bs ans;
        switch (datatype)
        {
        case GDT_Byte:{
            ans = rasterband_histogram_stretch<char>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Int16:{
            ans = rasterband_histogram_stretch<short>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_UInt16:{
            ans = rasterband_histogram_stretch<unsigned short>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Int32:{
            ans = rasterband_histogram_stretch<int>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_UInt32:{
            ans = rasterband_histogram_stretch<unsigned int>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Float32:{
            ans = rasterband_histogram_stretch<float>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Float64:{
            ans = rasterband_histogram_stretch<double>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        default:
            break;
        }

        if(!get<0>(ans)){
            return_msg(-3,get<1>(ans));
        }
    }

    return return_msg(1, EXE_NAME " end.");

}
