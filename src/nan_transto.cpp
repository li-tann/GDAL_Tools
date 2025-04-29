#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

#include <argparse/argparse.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "template_nan_convert_to.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{

    std::string str_nan = "nan";
    float val = stof(str_nan);
    
    cout<<val<<"\n"<< (isnan(val) ? "val is nan" : "val is not nan")<<endl;
    return 0;
    /// argparse
    argparse::ArgumentParser program("nan_TransTo","1.0", argparse::default_arguments::help);
    program.add_description("translate the NaN from in_img to out_img.");

    program.add_argument("input_image")
        .help("input imgpath, support datatype list: short, int, float, double");

    program.add_argument("output_image")
        .help("output imgpath (tiff)");

    program.add_argument("value")
        .help("input, which is NAN convert to")
        .scan<'g',double>();
    
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl<<std::endl;
        std::cerr << program;
        return 1;
    }
    
    /// log
    fs::path exe_root = fs::absolute(argv[0]).parent_path();
    fs::path log_path = exe_root / "LOG_nan_TransTo.log";
    auto my_logger = spdlog::basic_logger_mt("nan_TransTo", log_path.string());

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };


    GDALAllRegister();
    string msg;

    return_msg(0,"\nnan_TransTo start");

    
    double val = program.get<double>("value");

    string input = program.get<string>("input_image");
    string output= program.get<string>("output_image");

    /// 如果
    if(input!= output){
        fs::path src(input);
        fs::path dst(output);
        if(fs::exists(dst)){
            msg = "output is existed, we will remove and copy input to output.";
            return_msg(0,msg);
            fs::remove(dst);
        }
        fs::copy(src,dst);
    }

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(output.c_str(), GA_Update));
    if(ds == nullptr)
        return return_msg(-2,"ds is nullptr. output is no-existed(when input==output), or cann't be create?");

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
    return return_msg(1,"\nnan_TransTo succeed.");

}


    