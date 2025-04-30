#include "raster_include.h"

int image_set_colortable(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    std::string img_path = args->get<std::string>("img_path");
    std::string ct_path = args->get<std::string>("ct_path");

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(img_path.c_str(), GA_Update);
    if(!ds_in){
        PRINT_LOGGER(logger, error,"ds_in is nullptr.");
        return -1;
    }

    GDALColorTable gdal_ct;
    if(ct_path == "clear"){
        ds_in->GetRasterBand(1)->SetColorTable(NULL);
        GDALClose(ds_in);
        PRINT_LOGGER(logger, info,"image_set_colortable success (color table has been cleared).");
        return 1;
    }
    
    color_map cm2(ct_path.c_str());
    if(!cm2.is_opened()){
        PRINT_LOGGER(logger, error,"cm2 is open failed.");
        return -2;
    }

    for(int i=0; i<256; i++){
        int idx = i / 256.0 * cm2.node.size();
        GDALColorEntry ce;
        ce.c1 = cm2.color[idx].red;
        ce.c2 = cm2.color[idx].green;
        ce.c3 = cm2.color[idx].blue;
        ce.c4 = cm2.color[idx].alpha;
        gdal_ct.SetColorEntry(i, &ce);
    }

    PRINT_LOGGER(logger, info,fmt::format("cm2.node.size: {}", cm2.node.size()));

    auto err = ds_in->GetRasterBand(1)->SetColorTable(&gdal_ct);
    if(err != CE_None){
        PRINT_LOGGER(logger, error,"SetColorTable failed.");
        return -3;
    }


    GDALClose(ds_in);
    
    PRINT_LOGGER(logger, info,"image_set_colortable success.");
    return 1;
}

