#include <iostream>
#include <gdal_priv.h>
#include <fmt/format.h>

int read_tif(const char* path)
{
    GDALAllRegister();

    GDALDataset* ds = (GDALDataset*)GDALOpen(path, GA_ReadOnly);
    if(!ds){
        printf("ds is nullptr.\n");
        return -1;
    }

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    auto datatype = ds->GetRasterBand(1)->GetRasterDataType();

    std::cout<<"width:  "<<width<<std::endl;
    std::cout<<"height: "<<height<<std::endl;
    std::cout<<"datatype: "<<GDALGetDataTypeName(datatype)<<std::endl;

    double gt[6];
    ds->GetGeoTransform(gt);
    std::cout<<fmt::format("gt: {}/{}/{}/{}/{}/{}", gt[0], gt[1], gt[2], gt[3], gt[4], gt[5])<<std::endl;

    int* arr = new int[width*height];

    auto e = ds->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, GDT_Int32, 0, 0);
    if(e != CE_None){
        std::cout<<"rasterio failed, e:"<<e<<std::endl;
        return -1;
    }

    for(int i=0; i<height; i++){
        for(int j=0; j<width; j++){
            std::cout<<arr[i*width+j]<<(j == width -1 ? "\n" : ",");
        }
    }
    std::cout<<std::endl;
    GDALClose(ds);
    return 1;
}

int main()
{
    int* a = new int[6];
    for(int i=0; i<6; i++){
        a[i] = i*i;
    }

    /// @note 除了fmt 还可以使用gdal自己提供的指针打印功能
    char* tmp_ptr = new char[255];
    CPLPrintPointer(tmp_ptr, a, 255);
    std::cout<<"tmp_ptr:["<<tmp_ptr<<"]\n";

    std::string x = fmt::format(
        "MEM:::DATAPOINTER={:p},PIXELS={},LINES={},BANDS={},DATATYPE={},PIXELOFFSET={},LINEOFFSET={},BANDOFFSET={},GEOTRANSFORM={}/{}/{}/{}/{}/{}", 
        (void*)a,                                       /// DATAPOINTER
        2, 3, 1,                                        /// PIXELS, LINES, BANDS
        GDALGetDataTypeName(GDT_Int32),                 /// DATATYPE
        sizeof(int), sizeof(int) * 2, 1,                /// PIXELOFFSET, LINEOFFSET, BANDOFFSET
        112.0, 0.02, 0, 37.1, 0, -0.02                  /// GEOTRANSFORM
    );     ///

    std::cout<<x<<std::endl;
    return read_tif(x.c_str());
}