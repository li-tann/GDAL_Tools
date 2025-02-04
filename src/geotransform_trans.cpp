#include "raster_include.h"
/*
    sub_geo2pix.add_argument("geo_img")        
    sub_geo2pix.add_argument("-p","--pointlist").nargs(argparse::nargs_pattern::at_least_one);
*/

int cal_geo2pix(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string geo_img  = args->get<string>("geo_img");
    std::vector<std::string> pointlist = args->get<std::vector<std::string>>("--pointlist");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto ds = (GDALDataset*)GDALOpen(geo_img.c_str(), GA_ReadOnly);
    if(!ds){
        PRINT_LOGGER(logger, error, "geo2pix failed, ds is nullptr.");
        return -1;
    }

    double gt[6];
    auto cpl_rst = ds->GetGeoTransform(gt);
    if(cpl_rst != CPLErr::CE_None){
        PRINT_LOGGER(logger, error, "result of GetGeoTransform is not 'CE_None'.");
        GDALClose(ds);
        return -1;
    }

    PRINT_LOGGER(logger, info, fmt::format("geotransform: {}, {}, {}, {}, {}, {}",gt[0],gt[1],gt[2],gt[3],gt[4],gt[5]));


    int num = 0;
    for(auto& point_str : pointlist)
    {
        std::string logstr = fmt::format("{}th, ", num++);
        std::vector<std::string> point_str_vec;
        strSplit(point_str, point_str_vec, ",");

        if(point_str_vec.size() < 2){
            logstr += fmt::format("invalid (split.size < 2)");
            PRINT_LOGGER(logger, error, logstr);
            continue;
        }

        double lon = std::stod(point_str_vec[0]);
        double lat = std::stod(point_str_vec[1]);

        if(std::isnan(lon) || std::isnan(lat)){
            logstr += fmt::format("invalid (lon or lat is nan)");
            PRINT_LOGGER(logger, error, logstr);
            continue;
        }

        double pix_x = (lon - gt[0]) / gt[1];
        double pix_y = (lat - gt[3]) / gt[5];

        logstr += fmt::format("lon:{:>10f}, lat:{:>10f}, pix-x:{:10.3f}, pix-y:{:10.3f}", lon, lat, pix_x, pix_y);
        PRINT_LOGGER(logger, info, logstr);
    }
    

    PRINT_LOGGER(logger, info, "geo2pix finished.");
    return 1;
}


/*
    sub_pix2geo.add_argument("geo_img")        
    sub_pix2geo.add_argument("-p","--pointlist").nargs(argparse::nargs_pattern::at_least_one);
*/

int cal_pix2geo(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string geo_img  = args->get<string>("geo_img");
    std::vector<std::string> pointlist = args->get<std::vector<std::string>>("--pointlist");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto ds = (GDALDataset*)GDALOpen(geo_img.c_str(), GA_ReadOnly);
    if(!ds){
        PRINT_LOGGER(logger, error, "pix2geo failed, ds is nullptr.");
        return -1;
    }

    double gt[6];
    auto cpl_rst = ds->GetGeoTransform(gt);
    if(cpl_rst != CPLErr::CE_None){
        PRINT_LOGGER(logger, error, "result of GetGeoTransform is not 'CE_None'.");
        GDALClose(ds);
        return -1;
    }

    PRINT_LOGGER(logger, info, fmt::format("geotransform: {}, {}, {}, {}, {}, {}",gt[0],gt[1],gt[2],gt[3],gt[4],gt[5]));


    int num = 0;
    for(auto& point_str : pointlist)
    {
        std::string logstr = fmt::format("{}th, ", num++);
        std::vector<std::string> point_str_vec;
        strSplit(point_str, point_str_vec, ",");

        if(point_str_vec.size() < 2){
            logstr += fmt::format("invalid (split.size < 2)");
            PRINT_LOGGER(logger, error, logstr);
            continue;
        }

        double pix_y = std::stod(point_str_vec[0]);
        double pix_x = std::stod(point_str_vec[1]);

        if(std::isnan(pix_y) || std::isnan(pix_x)){
            logstr += fmt::format("invalid (pix_y or pix_x is nan)");
            PRINT_LOGGER(logger, error, logstr);
            continue;
        }

        double lon = gt[0] + gt[1] * pix_x;
        double lat = gt[3] + gt[5] * pix_y;

        logstr += fmt::format("pix-x:{:10.3f}, pix-y:{:10.3f}, lon:{:>10f}, lat:{:>10f}", pix_x, pix_y, lon, lat);
        PRINT_LOGGER(logger, info, logstr);
    }
    

    PRINT_LOGGER(logger, info, "pix2geo finished.");
    return 1;
}