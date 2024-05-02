#include "raster_include.h"
/*
    sub_vrt_to_tif.add_argument("vrt")
        .help("input image filepath (*.vrt)");

    sub_vrt_to_tif.add_argument("tif")
        .help("output image filepath (*.tif)");    

*/

int vrt_to_tif(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    string vrt_filepath = args->get<string>("vrt");
    string tif_filepath = args->get<string>("tif");

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(vrt_filepath.c_str(),GA_ReadOnly);
    if(!ds_in){
        PRINT_LOGGER(logger, error, "ds_in is nullptr");
        return -1;
    }
    GDALRasterBand* rb = ds_in->GetRasterBand(1);

    int width = ds_in->GetRasterXSize();
    int height = ds_in->GetRasterYSize();
    int bands = ds_in->GetRasterCount();
    GDALDataType datatype = rb->GetRasterDataType();
    int datasize = GDALGetDataTypeSize(datatype);

    GDALDriver* driver_tif = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = driver_tif->Create(tif_filepath.c_str(), width, height, bands, datatype,NULL);
    if(!ds_out){
        GDALClose(ds_in);
        PRINT_LOGGER(logger, error, "ds_out is nullptr");
        return -2;
    }
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);

    for(int b = 1; b <= bands; b++)
    {
        auto rb_in = ds_in->GetRasterBand(b);
        auto rb_out = ds_out->GetRasterBand(b);
        void* arr = malloc(width * datasize);
        for(int i=0; i<height; i++)
        {
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            rb_out->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
        }
        free(arr);
    }

    GDALClose(ds_in);
    GDALClose(ds_out);

    PRINT_LOGGER(logger, info,"vrt_to_tif success.");
    return 1;
}


/*
    sub_tif_to_vrt.add_argument("tif")
        .help("input image filepath");

    sub_tif_to_vrt.add_argument("binary")
        .help("output image filepath (binary, without extension)");
        
    sub_tif_to_vrt.add_argument("ByteOrder")
        .help("MSB or LSB");
*/

#include "binary_write.h"

#define IMG_WRITE(type) \
        binary_write<type> bw;                                                          \
        rst = bw.init(binary_filepath.c_str(), height, width, byte_order);              \
        if(!rst){                                                                       \
            PRINT_LOGGER(logger, error, fmt::format("bw<{}>.init failed.",#type));      \
            return -3;                                                                  \
        }                                                                               \
        type* arr = new type[width];                                                    \
        for(int i=0; i<height; i++){                                                    \
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);    \
            bw.array_to_bin(arr, width);                                                \
        }                                                                               \
        delete[] arr;                                                                   \
        rst = bw.print_vrt();                                                           \
        if(!rst){                                                                       \
            PRINT_LOGGER(logger, error, fmt::format("bw<{}>.print_vrt failed.",#type)); \
            return -4;                                                                  \
        }                                                                               \
        bw.close();                                                                     \

int tif_to_vrt(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    string binary_filepath = args->get<string>("binary");
    string vrt_filepath = binary_filepath + ".vrt";
    string tif_filepath = args->get<string>("tif");

    string str_byteorder = args->get<string>("ByteOrder");\
    std::transform(str_byteorder.begin(), str_byteorder.end(), str_byteorder.begin(), ::toupper);
    if(str_byteorder != "LSB" && str_byteorder != "MSB"){
        PRINT_LOGGER(logger, error, "str_byteorder is not 'LSB' or 'MSB'");
        return -1;
    }
    ByteOrder_ byte_order = (str_byteorder == "LSB" ? ByteOrder_::LSB : ByteOrder_::MSB );

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(tif_filepath.c_str(), GA_ReadOnly);
    if(!ds_in){
        PRINT_LOGGER(logger, error, "ds_in is nullptr");
        return -2;
    }
    GDALRasterBand* rb_in = ds_in->GetRasterBand(1);

    int width = ds_in->GetRasterXSize();
    int height = ds_in->GetRasterYSize();
    GDALDataType datatype = rb_in->GetRasterDataType();
    int datasize = GDALGetDataTypeSize(datatype);
    funcrst rst;

    switch (datatype)
    {
    case GDT_Byte:{
        IMG_WRITE(unsigned char);
    }break;    
    case GDT_Int16:{
        IMG_WRITE(short);
    }break;
    case GDT_Int32:{
        IMG_WRITE(int);
    }break;
    case GDT_Float32:{
        IMG_WRITE(float);
    }break;
    case GDT_Float64:{
        IMG_WRITE(double);
    }break;
    case GDT_CInt16:{
        IMG_WRITE(std::complex<short>);
    }break;
    case GDT_CInt32:{
        IMG_WRITE(std::complex<int>);
    }break;
    case GDT_CFloat32:{
        IMG_WRITE(std::complex<float>);
    }break;
    case GDT_CFloat64:{
        IMG_WRITE(std::complex<double>);
    }break;
    default:
        PRINT_LOGGER(logger, error, "unsupported datatype (supported datatype are: byte, short int, float, double, scpx, icpx, fcpx, dcpx)");
        return -3;
    }

    GDALClose(ds_in);

    PRINT_LOGGER(logger, info,"tif_to_vrt success.");
    return 1;
}
