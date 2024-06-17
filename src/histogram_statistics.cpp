#include "raster_include.h"
#include "template_stretch.h"

/*
    sub_histogram_stretch.add_argument("input_imgpath")
        .help("input image filepath, doesn't support complex datatype");

    sub_histogram_stretch.add_argument("histogram_size")
        .help("int, histogram size, more than 1, default is 256.")
        .scan<'i',int>()
        .default_value("256");    
*/

int histogram_statistics(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    std::string img_filepath = args->get<std::string>("input_imgpath");
    int histogram_length = args->get<int>("histogram_size");
    if(histogram_length < 1){
        PRINT_LOGGER(logger, error, fmt::format("histogram length({}) is less than 1.",histogram_length));
        return -1;
    }

    GDALDataset* ds = (GDALDataset*)GDALOpen(img_filepath.c_str(), GA_Update);
    if(!ds){
        PRINT_LOGGER(logger, error, "ds is nullptr.");
        return -2;
    }
        
    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();
    GDALDataType datatype = ds->GetRasterBand(1)->GetRasterDataType();
    if(datatype > GDT_Float64 /*cshort cint cfloat cdouble*/ || datatype == 0 /*unknown*/){
        PRINT_LOGGER(logger, error,"unsupported datatype, unsupport list: complex.");
        return -3;
    }
    GDALRasterBand* rb = ds->GetRasterBand(1);

    CPLErr err;
    double minmax[2];
    GUIntBig* histogram_result = new GUIntBig[histogram_length];
    err = rb->ComputeRasterMinMax(FALSE,minmax);
    if(err == CE_Failure){
        PRINT_LOGGER(logger, error,"rb.computeRasterMinMax failed.");
        return -4;
    }
    err = rb->GetHistogram(minmax[0], minmax[1], histogram_length, histogram_result, FALSE, FALSE, GDALDummyProgress, nullptr);
    if(err == CE_Failure){
        PRINT_LOGGER(logger, error,"rb.getHistrogram failed.");
        return -5;
    }

    double delta_value = ( minmax[1] -  minmax[0]) / histogram_length;

    string msg = "histogram statistics:\n";
    for(int i=0; i< histogram_length; i++)
    {
        double min = minmax[0] + i * delta_value;
        double max = minmax[0] + (i+1) * delta_value;
        unsigned long long num = histogram_result[i];
        msg += fmt::format("range: [{},{}], num:{}\n", min, max, num);
    }
    delete[] histogram_result;
    PRINT_LOGGER(logger, info, msg);

    PRINT_LOGGER(logger, info,"histogram_statistics success.");
    return 1;
}