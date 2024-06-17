#include "raster_include.h"
#include "template_stretch.h"
#define HISTOGRAM_SIZE 256

/*
    sub_histogram_stretch.add_argument("input_imgpath")
        .help("input image filepath, doesn't support complex datatype");

    sub_histogram_stretch.add_argument("output_imgpath")
        .help("output image filepath.");

    sub_histogram_stretch.add_argument("stretch_rate")
        .help("double, within (0,0.5], default is 0.02.")
        .scan<'h',double>()
        .default_value("0.2");    
*/

int histogram_stretch(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    std::string input_filepath = args->get<std::string>("input_imgpath");
    std::string output_filepath = args->get<std::string>("output_imgpath");
    double stretch_rate = args->get<double>("stretch_rate");


    if(input_filepath!= output_filepath){
        fs::path src(input_filepath);
        fs::path dst(output_filepath);
        if(fs::exists(dst)){
            PRINT_LOGGER(logger, warn,"input_file is existed, we will remove and copy input_file to output_file.");
            fs::remove(dst);
        }
        fs::copy(src,dst);
    }
    PRINT_LOGGER(logger, info, "input_file has been copy to output_file.");

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(output_filepath.c_str(), GA_Update));
    if(ds == nullptr){
        PRINT_LOGGER(logger, warn,"ds is nullptr. output_file is no-existed(when input_file==output_file), or cann't be create?");
        return -1;
    }
        
    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();
    GDALDataType datatype = ds->GetRasterBand(1)->GetRasterDataType();
    if(datatype > GDT_Float64 /*cshort cint cfloat cdouble*/ || datatype == 0 /*unknown*/){
        PRINT_LOGGER(logger, error,"unsupported datatype, unsupport list: complex");
        return -1;
    }

    for(int b = 1; b <= bands; b++)
    {
        GDALRasterBand* rb = ds->GetRasterBand(b);
        funcrst rst;
        switch (datatype)
        {
        case GDT_Byte:{
            rst = rasterband_histogram_stretch<char>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Int16:{
            rst = rasterband_histogram_stretch<short>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_UInt16:{
            rst = rasterband_histogram_stretch<unsigned short>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Int32:{
            rst = rasterband_histogram_stretch<int>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_UInt32:{
            rst = rasterband_histogram_stretch<unsigned int>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Float32:{
            rst = rasterband_histogram_stretch<float>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        case GDT_Float64:{
            rst = rasterband_histogram_stretch<double>(rb, b, xsize, ysize, HISTOGRAM_SIZE, stretch_rate);
        }break;
        default:
            break;
        }

        if(!rst){
            PRINT_LOGGER(logger, error, fmt::format("band[{}] rasterband_histogram_stretch failed.", b));
        }
    }

    return 1;
}