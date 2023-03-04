#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "template_nan_convert_to.h"

#define EXE_NAME "_nan_TransTo"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

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
                " argv[1]: input imgpath, support datatype list: short, int, float, double\n"
                " argv[2]: output imgpath,\n"
                " argv[3]: value, which is NAN convert to.";
        return return_msg(-1,msg);
    }

    msg = string(EXE_NAME) + " start.";
    return_msg(0,msg);

    double val = atof(argv[3]);

    /// 如果
    if(string(argv[1])!= string(argv[2])){
        fs::path src(argv[1]);
        fs::path dst(argv[2]);
        if(fs::exists(dst)){
            msg = "argv[2] is existed, we will remove and copy argv[1] to argv[2].";
            return_msg(0,msg);
            fs::remove(dst);
        }
        fs::copy(src,dst);
    }

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(argv[2],GA_Update));
    if(ds == nullptr)
        return return_msg(-2,"ds is nullptr. argv[2] is no-existed(when argv[1]==argv[2]), or cann't be create?");

    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();
    GDALDataType datatype = ds->GetRasterBand(1)->GetRasterDataType();

    tuple_bs rtn_msg;
    switch (datatype)
    {
    case GDT_Float32:
        rtn_msg = nodata_transto(ds,float(val));
        break;
    case GDT_Float64:
        rtn_msg = nodata_transto(ds,val);
        break;
    case GDT_Int16:
        rtn_msg = value_transto(ds,short(-32767),short(val));
        break;
    case GDT_Int32:
        rtn_msg = value_transto(ds,-32767,int(val));
        break;
    default:
        return return_msg(-3,"unsupport datatype.");
    }
    
    GDALClose(ds);
     msg = string(EXE_NAME) + " successed.";
    return return_msg(1, msg);

}


    