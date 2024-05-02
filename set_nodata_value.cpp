#include "raster_include.h"

/*
    sub_set_nodata.add_argument("img_filepath")
        .help("raster image filepath.");

    sub_set_nodata.add_argument("nodata_value")
        .help("nodata value")
        .scan<'g',double>();
*/

int set_nodata_value(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    std::string imgpath = args->get<std::string>("img_filepath");
    double val = args->get<double>("nodata_value");

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(imgpath.c_str(),GA_Update));
    if(ds == nullptr){
        PRINT_LOGGER(logger, error, "ds is nullptr.");
    }
    int bands = ds->GetRasterCount();


    for(int band = 1; band <= bands; band++)
    {
        GDALRasterBand* rb = ds->GetRasterBand(band);
        // err = rb_in->SetNoDataValue(val);
        auto err = rb->SetNoDataValue(val);
        if(err !=  CE_None){
            PRINT_LOGGER(logger, error, fmt::format("band[{}].setNoDataValue failed.", band));
            return false;
        }
    }
    GDALClose(ds);
    
    PRINT_LOGGER(logger, info, "set_nodata_value success.");
    return true;
}