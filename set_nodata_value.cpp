#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace std;

int main(int argc, char* argv[])
{
    GDALAllRegister();
    string msg;
    CPLErr err;

    auto my_logger = spdlog::basic_logger_mt("_Set_NoData_Value", "_Set_NoData_Value.txt");

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    if(argc < 3){
        msg =   "_Set_NoData_Value.exe\n"
                " manual:\n" 
                " argv[0]: _Set_NoData_Value,\n"
                " argv[1]: input imgpath,\n"
                " argv[2]: value, NoData Value.";
        return return_msg(-1,msg);
    }

    my_logger->info("_Set_NoData_Value start.");
    spdlog::info("_Set_NoData_Value start.");

    double val = atof(argv[2]);

    GDALDataset* ds_in = static_cast<GDALDataset*>(GDALOpen(argv[1],GA_ReadOnly));
    if(ds_in == nullptr)
        return return_msg(-2,"img_in.dataset is nullptr.");

    int xsize = ds_in->GetRasterXSize();
    int ysize = ds_in->GetRasterYSize();
    int bands = ds_in->GetRasterCount();
    GDALDataType datatype = ds_in->GetRasterBand(1)->GetRasterDataType();
    int sizeof_datatype = 0;

    for(int band = 1; band < bands; band++)
    {
        GDALRasterBand* rb_in = ds_in->GetRasterBand(band);
        // err = rb_in->SetNoDataValue(val);
        err = rb_in->SetNoDataValue(val);
        if(err > 1)
            return return_msg(-2,"setNoDataValue error.");
    }

    GDALClose(ds_in);
    

    return return_msg(1, "_Set_NoData_Value end.");

}
