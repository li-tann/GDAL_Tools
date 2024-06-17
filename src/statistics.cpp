#include "raster_include.h"

/*
    sub_statistics.add_argument("img_filepath")
        .help("raster image filepath.");

    sub_statistics.add_argument("band")
        .help("band, default is 1.")
        .scan<'i',int>()
        .default_value("1");
*/

int statistics(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string img_filepath = args->get<std::string>("img_filepath");
    int band = args->get<int>("band");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds = (GDALDataset*)GDALOpen(img_filepath.c_str(), GA_ReadOnly);
    if(ds == nullptr){
        PRINT_LOGGER(logger, error, "ds is nullptr");
        return -1;
    }
    int bands = ds->GetRasterCount();

    if(band > bands){
        PRINT_LOGGER(logger, error, fmt::format("band({}(input)) is bigger than ds.bands({})", band, bands));
        return -2;
    }

    GDALRasterBand* rb = ds->GetRasterBand(band);
        
    double min, max, mean, stddev;
    auto err = rb->ComputeStatistics(FALSE, &min, &max, &mean, &stddev, NULL,NULL);
    if(err != CE_None){
        PRINT_LOGGER(logger, error, "rb->ComputeStatistics failed.");
        GDALClose(ds);
        return -3;
    }
    std::string msg = "statistics info:\n"
    "band " + std::to_string(band) + ":\n"
    "minium :" + std::to_string(min) + "\n" +
    "maxium :" + std::to_string(max) + "\n" +
    "mean   :" + std::to_string(mean) + "\n" + 
    "stdDev :" + std::to_string(stddev);

    PRINT_LOGGER(logger, info, msg);

    GDALClose(ds);

    PRINT_LOGGER(logger, info, "set_nodata_value success.");
    return 1;
}