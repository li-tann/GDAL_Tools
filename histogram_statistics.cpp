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

#define EXE_NAME "histogram_statistics"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

struct range_statistics{
    double min{NAN}, max{NAN};
    int num=0;
    range_statistics(){}
    range_statistics(double min_, double max_, int num_):min(min_),max(max_),num(num_){}
};

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

    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input imgpath,\n"
                " argv[2]: histogram length, more than 1.";
        return return_msg(-1,msg);
    }

    int histogram_length = atoi(argv[2]);
    if(histogram_length < 1){
        return return_msg(-1, fmt::format("histogram length({}) is less than 1.",histogram_length));
    }
    
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
    GDALRasterBand* rb = ds->GetRasterBand(1);

    CPLErr err;
    double minmax[2];
    GUIntBig* histogram_result = new GUIntBig[histogram_length];
    err = rb->ComputeRasterMinMax(FALSE,minmax);
    if(err == CE_Failure){
        return return_msg(-2,"rb.computeRasterMinMax failed.");
    }
    err = rb->GetHistogram(minmax[0], minmax[1], histogram_length, histogram_result, FALSE, FALSE, GDALDummyProgress, nullptr);
    if(err == CE_Failure){
        return return_msg(-2 ,"rb.getHistrogram failed.");
    }

    
    double delta_value = ( minmax[1] -  minmax[0]) / histogram_length;
    cout<<"histogram statistics,"<<endl;
    for(int i=0; i< histogram_length; i++)
    {
        double min = minmax[0] + i * delta_value;
        double max = minmax[0] + (i+1) * delta_value;
        unsigned long long num = histogram_result[i];
        cout<<fmt::format("range: [{},{}]", min, max)<<endl;
        cout<<fmt::format(" num : {}",num)<<endl;
    }

    delete[] histogram_result;

    return return_msg(1, EXE_NAME " end.");

}
