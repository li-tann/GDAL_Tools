#include "raster_include.h"

/*
    sub_trans_geoinfo.add_argument("source")
        .help("source image");

    sub_trans_geoinfo.add_argument("target")
        .help("target image");    
*/

int trans_geoinformation(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    string src = args->get<string>("source");
    string dst = args->get<string>("target");

    GDALDataset* ds_in = static_cast<GDALDataset*>(GDALOpen(src.c_str(), GA_ReadOnly));
    if(!ds_in){
        PRINT_LOGGER(logger, error, "ds_in is nullptr.");
        return -1;
    }
        
    GDALDataset* ds_out = static_cast<GDALDataset*>(GDALOpen(dst.c_str(), GA_Update));
    if(!ds_out){
        PRINT_LOGGER(logger, error, "ds_out is nullptr.");
        return -2;
    }
    
    double gt[6];
    auto err = ds_in->GetGeoTransform(gt);
    // std::cout<<fmt::format("gt: {:>8f}, {:>8f}, {:>8f}, {:>8f}, {:>8f}, {:>8f}", gt[0], gt[1], gt[2], gt[3], gt[4], gt[5])<<std::endl;
    // std::cout<<"GetGeoTransform return :"<<err<<std::endl;
    if(err != CPLErr::CE_None){
        std::cout<<"err != CPLErr::CE_None\n";
        PRINT_LOGGER(logger, info, "ds_in.GetGeoTransform failed, so ds_out.SetGeoTransform({0,0,0,0,0,0}).");
        gt[0]=0; gt[1]=0; gt[2]=0; gt[3]=0; gt[4]=0; gt[5]=0; 
    }
    ds_out->SetGeoTransform(gt);

    ds_out->SetProjection(ds_in->GetProjectionRef());
    
    GDALClose(ds_in);
    GDALClose(ds_out);
    
    PRINT_LOGGER(logger, info, "trans_geoinformation success.");
    return 1;
}