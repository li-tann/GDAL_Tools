#include "raster_include.h"

#include <string>
#include <filesystem>
namespace fs = std::filesystem;

int data_convert_to_byte(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    std::string img_path = args->get<std::string>("img_path");
    std::string out_path = args->get<std::string>("out_path");
    std::string extension = args->get<std::string>("extension"); /// png jpg tif bmp
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if(extension != "png" && extension != "jpg" && extension != "tif" && extension != "bmp"){
        PRINT_LOGGER(logger, error,fmt::format("un-supported extension '{}'",extension));
        return -1;
    }

    std::string gdal_driver_str = "";
    int bandCount = 1;
    if(extension == "png"){
        gdal_driver_str = "PNG";
        bandCount = 3;
    }
    else if(extension == "jpg"){
        gdal_driver_str = "JPEG";
        bandCount = 3;
    }
    else if(extension == "tif"){
        gdal_driver_str = "GTiff";
    }
    else if(extension == "bmp"){
        gdal_driver_str = "BMP";
    }
    else{
        PRINT_LOGGER(logger, error,fmt::format("un-supported extension '{}'",extension));
        return -1;
    }
    GDALDriver* dri_out = GetGDALDriverManager()->GetDriverByName(gdal_driver_str.c_str());
    if(!dri_out){
        PRINT_LOGGER(logger, error,fmt::format("un-supported driver '{}'",gdal_driver_str));
        return -1;
    }



    fs::path p_out(out_path);
    if(p_out.extension().string() != ("." + extension)){
        std::string old_extension = p_out.extension().string();
        p_out.replace_extension("." + extension);
        out_path = p_out.string();
        PRINT_LOGGER(logger, warn,fmt::format("out_path.extension ({}) is diff with extension ({}), out_path has been convert to '{}'",old_extension, extension, out_path));
    }

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(img_path.c_str(), GA_ReadOnly);
    if(!ds_in){
        PRINT_LOGGER(logger, error, "ds_in is nullptr.");
        return -1;
    }
    int width = ds_in->GetRasterXSize();
    int height= ds_in->GetRasterYSize();
    GDALRasterBand* rb_in = ds_in->GetRasterBand(1);
    GDALDataType datatype = rb_in->GetRasterDataType();
    int datasize = GDALGetDataTypeSize(datatype);
    if(datatype > 7 || datatype == 0){ /// 1(byte), 2(ushort), 3(short), 4(uint), 5(int), 6(float), 7(double)
        GDALClose(ds_in);
        PRINT_LOGGER(logger, error, fmt::format("datatype is complex({}), error.",GDALGetDataTypeName(datatype)));
        return -1;
    }

    double minmax[2];
    auto err = rb_in->ComputeRasterMinMax(FALSE,minmax);
    if(err == CE_Failure){
        PRINT_LOGGER(logger, error,"rb.computeRasterMinMax failed.");
        return -2;
    }

    auto value_map = [&minmax](double value) -> unsigned char{
        unsigned char tmp;
        if(minmax[1] == minmax[0]){
            tmp = 0;
        }
        else{
            tmp = (value - minmax[0]) / ( minmax[1] -  minmax[0]) * 255;
        }

        return tmp;
    };

    GDALDriver* dri_mem = GetGDALDriverManager()->GetDriverByName("MEM");
    GDALDataset* ds_mem = dri_mem->Create("", width, height, bandCount, GDT_Byte, NULL);
    
    unsigned char* arr_out = new unsigned char[width];
    switch (datatype)
    {
    case GDT_Byte:{
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr_out, width, 1, datatype, 0, 0);
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
            
        }

    }break;
    case GDT_UInt16:{
        unsigned short* arr = new unsigned short[width];
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr_out[j] = value_map(arr[j]);
            }
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
        }
        delete[] arr;
    }break;
    case GDT_Int16:{
        short* arr = new short[width];
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr_out[j] = value_map(arr[j]);
            }
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
        }
        delete[] arr;
    }break;
    case GDT_UInt32:{
        unsigned int* arr = new unsigned int[width];
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr_out[j] = value_map(arr[j]);
            }
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
        }
        delete[] arr;
    }break;
    case GDT_Int32:{
        int* arr = new int[width];
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr_out[j] = value_map(arr[j]);
            }
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
        }
        delete[] arr;
    }break;
    case GDT_Float32:{
        float* arr = new float[width];
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr_out[j] = value_map(arr[j]);
            }
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
        }
        delete[] arr;
    }break;
    case GDT_Float64:{
        double* arr = new double[width];
        for(int i=0; i<height; i++){
            rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr_out[j] = value_map(arr[j]);
            }
            for(int b=1; b<= bandCount; b++){
                ds_mem->GetRasterBand(b)->RasterIO(GF_Write, 0, i, width, 1, arr_out, width, 1, GDT_Byte, 0, 0);
            }
        }
        delete[] arr;
    }break;
    default:
        break;
    }
    
    delete[] arr_out;
    GDALClose(ds_in);

    char** papszOptions = nullptr;
    if(gdal_driver_str == "JPEG"){
        papszOptions = CSLSetNameValue(nullptr, "QUALITY", "100");
        PRINT_LOGGER(logger, info, "set 'QUALITY VAL' as 100");
    }
    GDALDataset* ds_out = dri_out->CreateCopy(out_path.c_str(), ds_mem, true, papszOptions, nullptr, nullptr);
    if(!ds_out){
        GDALClose(ds_mem);
        PRINT_LOGGER(logger, error,"ds_out is nullptr");
        return -3;
    }

    GDALClose(ds_mem);
    GDALClose(ds_out);
    
    PRINT_LOGGER(logger, info,"data_convert_to_byte success.");
    return 1;
}