#include "raster_include.h"
#include "template_nan_convert_to.h"
/*
        sub_value_translate.add_argument("input_imgpath")
            .help("the original image with the source value.");

        sub_value_translate.add_argument("output_imgpath")
            .help("the target image, 如果output_imgpath == input_imgpath, 则直接对源数据进行操作.");

        sub_value_translate.add_argument("source_value")
            .help("老版本的数据, 需要保证是一个数字")
            .scan<'g',double>();
        
        sub_value_translate.add_argument("target_value")
            .help("转换后的数据, 需要保证是一个数字")
            .scan<'g',double>();
*/

int value_translate(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_filepath = args->get<std::string>("input_imgpath");
    std::string output_filepath = args->get<std::string>("output_imgpath");
    double input_val = args->get<double>("source_value");
    double output_val = args->get<double>("target_value");

    if(input_filepath!= output_filepath){
        fs::path src(input_filepath);
        fs::path dst(output_filepath);
        if(fs::exists(dst)){
            PRINT_LOGGER(logger, warn, "output is existed, we will remove and copy input to output.");
            fs::remove(dst);
        }
        fs::copy(src,dst);
    }
    PRINT_LOGGER(logger, info, "input_file has been copy to output_file.");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(output_filepath.c_str(), GA_Update));
    if(ds == nullptr){
        PRINT_LOGGER(logger, error,"ds is nullptr. output is no-existed(when input==output), or cann't be create?");
        return -1;
    }
        
    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();
    GDALDataType datatype = ds->GetRasterBand(1)->GetRasterDataType();
    PRINT_LOGGER(logger, info, fmt::format("the basic info of output_file: width:{}, height:{}, datatype:{}", xsize, ysize, GDALGetDataTypeName(datatype)));

    funcrst rst;
    switch (datatype)
    {
    case GDT_Float32:{
        float in_val = input_val, out_val = output_val;
        PRINT_LOGGER(logger, info,fmt::format("input_value:[{}], output_value:[{}]", in_val, out_val));
        rst = value_transto(ds, in_val, out_val);
    }break;
    case GDT_Float64:{
        double in_val = input_val, out_val = output_val;
        PRINT_LOGGER(logger, info,fmt::format("input_value:[{}], output_value:[{}]", in_val, out_val));
        rst = value_transto(ds,in_val, out_val);
    }break;
    case GDT_Int16:{
        short in_val = input_val, out_val = output_val;
        PRINT_LOGGER(logger, info,fmt::format("input_value:[{}], output_value:[{}]", in_val, out_val));
        rst = value_transto(ds,in_val,out_val);
    }break;
    case GDT_Int32:{
        int in_val = input_val, out_val = output_val;
        PRINT_LOGGER(logger, info,fmt::format("input_value:[{}], output_value:[{}]", in_val, out_val));
        rst = value_transto(ds, in_val, out_val);
    }break;
    default:
        PRINT_LOGGER(logger, error,"unsupport datatype. (not float, double, short, int)");
        return -2;
    }
    if(!rst){
        PRINT_LOGGER(logger, error,fmt::format("template function 'value_transto', return false, cause '{}'",rst.explain));
        return -3;
    }
    
    GDALClose(ds);
    PRINT_LOGGER(logger, info,"value_translate success.");
    return 0;

}