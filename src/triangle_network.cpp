#include "raster_include.h"

/*
    sub_band_extract.add_argument("input")
        .help("raster image (dem), with short or float datatype.");

*/
#include <triangle.h>

struct point_st
{
    double x,y;
};

struct triangle_st
{
    int c[3];
};


int triangle_newwork(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("mask");
	double thres  = args->get<double>("val_thres");
    std::string output_type  = args->get<string>("output_type");


    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto dataset = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly());
    if(!dataset){
        PRINT_LOGGER(logger, error, "dataset is nullptr.");
        return -1;
    }

    int width = dataset->GetRasterXSize();
    int height = dataset->GetRasterYSize();

    auto band = dataset->GetRasterBand(1);
    auto datatype = band->GetRasterDataType();

    if(datatype != GDT_Byte){
        PRINT_LOGGER(logger, error, "datatype is not byte.");
        GDALClose(dataset);
        return -1;
    }

    /// @note mask
    char* arr = new char[width * height];
    band->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, GDT_Byte, 0, 0);
    int valid_num = 0;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            valid_num += arr[i*width+j];
        }
    }

    /// @note init triangulate tr1
    triangulateio tr1, tr2, tr3;
    tr1.numberofcorners = valid_num;
    tr1.pointlist = new double(2 * valid_num);

    int k=0;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            tr1.pointlist[k++] = j;
            tr1.pointlist[k++] = i;
        }
    }

    triangulate("zcpqD", &tr1, &tr2, &tr3);

    int num_triangles = tr2.numberoftriangles;
    triangle_st* tr = (triangle_st*)tr2.trianglelist;

    int num_points = tr1.numberofpoints;
    point_st* x = (point_st*)tr1.pointlist;

    delete[] tr2.edgemarkerlist;
    


    PRINT_LOGGER(logger, info, "import_points_extract finished.");
    return 1;
}
