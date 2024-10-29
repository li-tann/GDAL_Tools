#include "raster_include.h"

int jpg_to_png(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    /// jpg 地址
    std::string jpg_path = args->get<std::string>("jpg");
    /// png 地址
    std::string png_path = args->get<std::string>("png");
    /// -a alpha 如果有-a, png就是4个波段, 并且读-a后面的参数(如果有)
    bool used_alpha = args->is_used("--alpha");
    rgba nodata;
    if(used_alpha){
        std::string nodata_rgba = args->get<std::string>("--alpha");
        rgba tmp(nodata_rgba);
        std::cout<<tmp.rgba_to_hex();
        if(nodata == tmp){
            PRINT_LOGGER(logger, error,"--alpha to rgba failed.");
            return -1;
        }
    }

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_jpg = (GDALDataset*)GDALOpen(jpg_path.c_str(), GA_ReadOnly);
    if(!ds_jpg){
        PRINT_LOGGER(logger, error,"ds_jpg is nullptr.");
        return -2;
    }
    int width = ds_jpg->GetRasterXSize();
    int height= ds_jpg->GetRasterYSize();
    int bands = ds_jpg->GetRasterCount();
    if(bands < 3){
        PRINT_LOGGER(logger, error,"ds_jpg.bands less than 3.");
        GDALClose(ds_jpg);
        return -2;
    }
    if(GDT_Byte != ds_jpg->GetRasterBand(1)->GetRasterDataType()){
        PRINT_LOGGER(logger, error,"ds_jpg.datatype is not gdt_byte.");
        GDALClose(ds_jpg);
        return -2;
    }

    /// mem dst
    GDALDriver* dri_mem = GetGDALDriverManager()->GetDriverByName("MEM");
    GDALDataset* ds_mem = dri_mem->Create("", width, height, used_alpha ? 4 : 3, GDT_Byte, nullptr);
    if(!ds_mem){
        GDALClose(ds_jpg);
        PRINT_LOGGER(logger, error,"ds_mem is nullptr.");
        return -3;
    }

    unsigned char* arr_r = new unsigned char[width];
    unsigned char* arr_g = new unsigned char[width];
    unsigned char* arr_b = new unsigned char[width];

    unsigned char* arr_a = new unsigned char[width];

    for(int i=0; i<height; i++)
    {
        ds_jpg->GetRasterBand(1)->RasterIO(GF_Read, 0, i, width, 1, arr_r, width, 1, GDT_Byte, 0, 0);
        ds_jpg->GetRasterBand(2)->RasterIO(GF_Read, 0, i, width, 1, arr_g, width, 1, GDT_Byte, 0, 0);
        ds_jpg->GetRasterBand(3)->RasterIO(GF_Read, 0, i, width, 1, arr_b, width, 1, GDT_Byte, 0, 0);
#pragma omp parallel for
        for(int j=0; j<width; j++)
        {
            if(used_alpha){
                arr_a[j] = 255;
                if( arr_r[j] == nodata.red && arr_g[j] == nodata.green &&arr_b[j] == nodata.blue)
                    arr_a[j] = 0;
            }
        }
        ds_mem->GetRasterBand(1)->RasterIO(GF_Write, 0, i, width, 1, arr_r, width, 1, GDT_Byte, 0, 0);
        ds_mem->GetRasterBand(2)->RasterIO(GF_Write, 0, i, width, 1, arr_g, width, 1, GDT_Byte, 0, 0);
        ds_mem->GetRasterBand(3)->RasterIO(GF_Write, 0, i, width, 1, arr_b, width, 1, GDT_Byte, 0, 0);
        if(used_alpha){
            ds_mem->GetRasterBand(4)->RasterIO(GF_Write, 0, i, width, 1, arr_a, width, 1, GDT_Byte, 0, 0);
        }
    }

    GDALDriver* dri_png = GetGDALDriverManager()->GetDriverByName("PNG");
    GDALDataset* ds_png = dri_png->CreateCopy(png_path.c_str(), ds_mem, false, nullptr, nullptr, nullptr);
    if(!ds_png){
        GDALClose(ds_jpg);GDALClose(ds_mem);
        PRINT_LOGGER(logger, error,"ds_png is nullptr");
        return -4;
    }
    GDALClose(ds_png);
    GDALClose(ds_jpg);
    GDALClose(ds_mem);

    delete[] arr_r;
    delete[] arr_g;
    delete[] arr_b;
    delete[] arr_a;


    PRINT_LOGGER(logger, info, "jpg_to_png success.");
    return 1;
}