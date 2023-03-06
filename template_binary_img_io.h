#ifndef TEMPLATE_BINARY_IMG_IO
#define TEMPLATE_BINARY_IMG_IO

#include <iostream>
#include <fstream>

#include <gdal_priv.h>

using tuple_bs = std::tuple<bool, std::string>;

/// 还没有测试

/// arr 不需要定义长度
template<typename _Ty>
bool load_binary_to_arr(const char* binary_imgpath, int width, int height, _Ty* arr)
{
    using namespace std;
    if(arr != nullptr){
        delete[] arr;
        arr = new _Ty[width * height];
    }

    ifstream ifs(binary_imgpath,ifstream::binary);
    if(!ifs.is_open()){
        return false;
    }

    size_t sizeof_Ty = sizeof(_Ty);
    int num = 0;
    _Ty val_init;
    while (ifs.read((char*)&val_init, sizeof_Ty)) { //一直读到文件结束
        //高低位字节变换
        _Ty val_trans;
        for(int i=0; i< sizeof_Ty; i++)
        {
            int j = sizeof_Ty - 1 - i;
            ((char*)&val_trans)[j] = ((char*)&val_init)[i];
        }
        arr[num++] = val_trans;
    }
}

template<typename _Ty>
inline _Ty binary_swap(_Ty src)
{
    _Ty dst;
    size_t src_size = sizeof(_Ty);
    unsigned char* src_uc = (unsigned char*)(&src);
    unsigned char* dst_uc = (unsigned char*)(&dst);
    for (size_t i = 0; i < src_size; i++) {
        dst_uc[i] = src_uc[src_size - 1 - i];
    }
    return dst;
}

template<typename _Ty>
bool binary_to_array(const char* binary_imgpath, int width, int height, _Ty* arr, bool swap = true)
{
    using namespace std;
    if(arr != nullptr){
        delete[] arr;
        arr = new _Ty[width * height];
    }

    ifstream ifs(binary_imgpath,ifstream::binary);
    if(!ifs.is_open()){
        return false;
    }

    size_t sizeof_Ty = sizeof(_Ty);
    int num = 0;
    _Ty val_init;
    while (ifs.read((char*)&val_init, sizeof_Ty) || num <= width * height) { //一直读到文件结束
        //高低位字节变换
        arr[num++] = swap ? binary_swap(val_init) : val_trans;
    }
    ifs.close();
    return true;
}

template<typename _Ty>
bool array_to_binary(_Ty* array, int array_size, const char* binary_imgpath)
{
    using namespace std;
    ofstream ofs(binary_imgpath, ofstream::binary);
    if(!ofs.is_open()){
        return false;
    }

    for(int i=0; i< array_size; i++){
        ofs.write((char*)(array[i]),sizeof(_Ty));
    }

    ofs.close();
    return true;
}



template<typename _Ty>
class binary_tif_io
{
public:
    _Ty* array;
    int width; 
    int height;

    binary_tif_io(int w, int h):width(w),height(h){array = new _Ty[width * height];};
    ~binary_tif_io(){if(array != nullptr)delete[] array;}

    tuple_bs read_binary(const char* filepath, bool b_swap = true)
    {
        using namespace std;

        ifstream ifs(filepath,ifstream::binary);
        if(!ifs.is_open()){
            return make_tuple(false,"ifs.open failed.");
        }

        size_t sizeof_Ty = sizeof(_Ty);
        int num = 0;
        _Ty val_init;
        while (ifs.read((char*)&val_init, sizeof_Ty)) { //一直读到文件结束
            //高低位字节变换
            array[num++] = b_swap ? binary_swap(val_init) : val_init;
            if(num >= width * height)break;
        }
        ifs.close();
        return make_tuple(true,"read_binary succeed.");
    }
    bool write_binary(const char* filepath)
    {
        using namespace std;
        ofstream ofs(filepath, ofstream::binary);
        if(!ofs.is_open()){
            return false;
        }

        for(int i=0; i< width * height; i++){
            ofs.write((char*)(array[i]),sizeof(_Ty));
        }

        ofs.close();
        return true;
    }

    bool read_tif(const char* filepath)
    {
        GDALAllRegister();
        GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(filepath,GA_ReadOnly));
        if(ds == nullptr || ds->GetRasterXSize()!=width || ds->GetRasterYSize() != height){
            return false;
        }
        
        GDALRasterBand* rb = ds->GetRasterBand(1);
        CPLErr err = rb->RasterIO(GF_Read,0,0,width,height,array,width,height,rb->GetRasterDataType(),0,0);
        GDALClose(ds);
        if(err == CE_Failure){
            return false;
        }
        return true;
    }

    tuple_bs write_tif(const char* filepath, GDALDataType dt)
    {
        using namespace std;
        GDALAllRegister();
        GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
        GDALDataset* ds = driver->Create(filepath, width, height, 1, dt, nullptr);
        if(ds == nullptr){
            return make_tuple(false,"dataset is nullptr.");
        }

        GDALRasterBand* rb = ds->GetRasterBand(1);
        CPLErr err = rb->RasterIO(GF_Write,0,0,width,height,array,width,height,dt,0,0);
        GDALClose(ds);
        if(err == CE_Failure){
            return make_tuple(false,"rasterband.rasterio.write failed.");
        }
        return make_tuple(true,"write_tif succeed.");;

    }

};

#endif //TEMPLATE_BINARY_IMG_IO
