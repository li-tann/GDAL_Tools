#include "raster_include.h"

/*
    argparse::ArgumentParser sub_band_extract("band_extract");
    sub_band_extract.add_description("extract a single band and create a tif format data.");
    {
        sub_band_extract.add_argument("src")
            .help("src image");

        sub_band_extract.add_argument("bands")
            .help("bands number list, like: 1 2 3... ")
            .scan<'i',int>()
            .nargs(argparse::nargs_pattern::at_least_one);
    }
*/

int band_extract(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string src_path  = args->get<string>("src");
	auto bands = args->get<vector<int>>("bands");
    
    fs::path src_p(src_path);
    if(!fs::exists(src_p)){
        PRINT_LOGGER(logger, error, "src_path is not existed.");
        return -1;
    }

    GDALAllRegister();
    auto ds_src = (GDALDataset*)GDALOpen(src_path.c_str(), GA_ReadOnly);
    if(!ds_src){
        PRINT_LOGGER(logger, error, "ds_src is nullptr");
        return -2;
    }
    int width = ds_src->GetRasterXSize();
    int height= ds_src->GetRasterYSize();

    auto driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    double gt[6];
    ds_src->GetGeoTransform(gt);
    PRINT_LOGGER(logger, info, fmt::format("ds_src.band is {}.", ds_src->GetRasterCount()));
    PRINT_LOGGER(logger, info, fmt::format("bands to extract is {}.", fmt::join(bands,", ")));

    for(auto b : bands)
    {
        fs::path new_path(src_path);
        new_path.replace_extension(fmt::format("band_{}.tif",b));

        auto rb = ds_src->GetRasterBand(b);
        if(!rb){
            PRINT_LOGGER(logger, error, fmt::format("band {}, is invalid",b));
            continue;
        }
        auto datatype = rb->GetRasterDataType();

        auto ds_out = driver->Create(new_path.string().c_str(), width, height, 1, datatype, NULL);
        auto rb_out = ds_out->GetRasterBand(1);

        ds_out->SetGeoTransform(gt);
        ds_out->SetProjection(ds_src->GetProjectionRef());

        void* arr = malloc(width * GDALGetDataTypeSize(datatype));

        for(int i=0; i<height; i++)
        {
            rb->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
        }
        
        free(arr);
        GDALClose(ds_out);
        PRINT_LOGGER(logger, info, fmt::format("band {}, extract success.",b));
    }

    GDALClose(ds_src);
    return 1;
}