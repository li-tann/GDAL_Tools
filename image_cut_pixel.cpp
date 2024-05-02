#include "raster_include.h"

/*
    sub_image_cut_pixel.add_argument("input_imgpath")
        .help("input image filepath");

    sub_image_cut_pixel.add_argument("output_imgpath")
        .help("output image filepath");   

    sub_image_cut_pixel.add_argument("pars")
        .help("4 pars with the order like: start_x start_y width height")
        .scan<'i',int>()
        .nargs(4);       
*/

int image_cut_by_pixel(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");


    string input_img = args->get<string>("input_imgpath");
    string output_img = args->get<string>("output_imgpath");
    vector<int> pars_list = args->get<vector<int>>("pars");
    int start_x = pars_list[0];
    int start_y = pars_list[1];
    int cutted_width  = pars_list[2];
    int cutted_height = pars_list[3];

    GDALDataset* ds = (GDALDataset*)GDALOpen(input_img.c_str(), GA_ReadOnly);
    if(!ds){
        PRINT_LOGGER(logger, error, "ds is nullptr.");
        return -1;
    }
    GDALRasterBand* rb = ds->GetRasterBand(1);

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    int bands = ds->GetRasterCount();
    GDALDataType datatype = rb->GetRasterDataType();
    int datasize = GDALGetDataTypeSize(datatype);
    double geotransform[6];
    auto cplerr = ds->GetGeoTransform(geotransform);
    bool has_geotransform = (cplerr == CE_Failure ? false : true);

    /// 调整裁剪范围
    if(start_x < 0) start_x = 0;
    if(start_y < 0) start_y = 0;
    if(start_x + cutted_width > width) cutted_width = width - start_x;
    if(start_y + cutted_height > height) cutted_height = height - start_y;


    GDALDriver* dv = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dv->Create(output_img.c_str(), cutted_width, cutted_height, bands, datatype, NULL);
    if(!ds_out){
        PRINT_LOGGER(logger, error, "ds_out is nullptr, create failed.");
        return -1;
    }

    for(int b = 1; b <= bands; b++)
    {
        GDALRasterBand* rb_out = ds_out->GetRasterBand(b);
        void* arr = malloc(cutted_width * datasize);
        for(int i=0; i< cutted_width; i++)
        {
            rb->RasterIO(GF_Read, start_x, i + start_y, cutted_width, 1, arr, cutted_width, 1, datatype, 0, 0);
            rb_out->RasterIO(GF_Write, 0, i, cutted_width, 1, arr, cutted_width, 1, datatype, 0, 0);
        }
        free(arr);
    }

    if(has_geotransform){
        double geotransform_out[6];
        geotransform_out[0] = geotransform[0] + start_x * geotransform[1] + start_y * geotransform[2];
        geotransform_out[1] = geotransform[1];
        geotransform_out[2] = geotransform[2];
        geotransform_out[3] = geotransform[3] + start_x * geotransform[4] + start_y * geotransform[5];
        geotransform_out[4] = geotransform[4];
        geotransform_out[5] = geotransform[5];
        ds_out->SetGeoTransform(geotransform_out);
        ds_out->SetProjection(ds->GetProjectionRef());
    }

    GDALClose(ds);
    GDALClose(ds_out);

    PRINT_LOGGER(logger, info, "image_cut_by_pixel success.");
    return 1;
}