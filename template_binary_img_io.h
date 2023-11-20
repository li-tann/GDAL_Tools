#ifndef TEMPLATE_BINARY_IMG_IO
#define TEMPLATE_BINARY_IMG_IO

#include <iostream>
#include <fstream>
#include <complex>

#include <gdal_priv.h>

using tuple_bs = std::tuple<bool, std::string>;

// template<typename _Ty>
// inline _Ty binary_swap(_Ty src)
// {
//     _Ty dst;
//     size_t src_size = sizeof(_Ty);
//     unsigned char* src_uc = (unsigned char*)(&src);
//     unsigned char* dst_uc = (unsigned char*)(&dst);
//     for (size_t i = 0; i < src_size; i++) {
//         dst_uc[i] = src_uc[src_size - 1 - i];
//     }
//     return dst;
// }

template<typename _Ty>
class binary_tif_io
{
public:
    _Ty* array;
    int width; 
    int height;

    binary_tif_io(int w, int h):width(w),height(h){array = new _Ty[width * height];};
    ~binary_tif_io(){if(array != nullptr)delete[] array;}

    _Ty binary_swap(_Ty src)
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
        while (ifs.read((char*)&val_init, sizeof_Ty)) { 
            //high-low bytes trans
            array[num++] = b_swap ? binary_swap(val_init) : val_init;
            if(num >= width * height)break;
        }
        ifs.close();
        return make_tuple(true,"read_binary succeed.");
    }
    
    tuple_bs write_binary(const char* filepath, bool b_swap = true)
    {
        using namespace std;
        ofstream ofs(filepath, ofstream::binary);
        if(!ofs.is_open()){
            return make_tuple(false,"ofs.open failed.");
        }

        for(int i=0; i< width * height; i++){
            _Ty val = (b_swap ? binary_swap(array[i]) : array[i]);
            ofs.write((char*)(&val),sizeof(_Ty));
        }

        ofs.close();
        return make_tuple(true,"write_binary succeed.");
    }

    tuple_bs read_tif(const char* filepath)
    {
        using namespace std;
        GDALAllRegister();
        GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(filepath,GA_ReadOnly));
        if(ds == nullptr){
            return make_tuple(false, "dataset is nullptr.");
        }
        if(ds->GetRasterXSize()!=width || ds->GetRasterYSize() != height){
            GDALClose(ds);
            return make_tuple(false, "ds.size is diff with width or height.");
        }
        
        GDALRasterBand* rb = ds->GetRasterBand(1);
        CPLErr err = rb->RasterIO(GF_Read,0,0,width,height,array,width,height,rb->GetRasterDataType(),0,0);
        GDALClose(ds);
        if(err == CE_Failure){
            return make_tuple(false,"rasterband.rasterio.read failed.");
        }
        return make_tuple(true,"read_tif succeed.");;
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

/// @brief 
/// @tparam _Ty if datatype we have is fcomplex, then print float in _Ty
template<typename _Ty>
class binary_tif_io_cpx
{
public:
    std::complex<_Ty>* array;
    int width; 
    int height;

    binary_tif_io_cpx(int w, int h):width(w),height(h){array = new std::complex<_Ty>[width * height];};
    ~binary_tif_io_cpx(){if(array != nullptr)delete[] array;}

    _Ty binary_swap(_Ty src)
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
    
    tuple_bs read_binary_cpx(const char* filepath, bool b_swap = true)
    {
        using namespace std;
        ifstream ifs(filepath,ifstream::binary);
        if(!ifs.is_open()){
            return make_tuple(false,"ifs.open failed.");
        }

        size_t sizeof_Ty = sizeof(_Ty);
        int num = 0;
        _Ty val_init;
        bool isReal {true}; // isReal -- isImag
        while (ifs.read((char*)&val_init, sizeof_Ty)) { 
            //high-low bytes trans
            if(isReal){
                array[num].real(b_swap ? binary_swap(val_init) : val_init);
            }else{
                array[num++].imag(b_swap ? binary_swap(val_init) : val_init);
            }
            isReal = !isReal;
                
            if(num >= width * height)break;
        }
        ifs.close();
        return make_tuple(true,"read_binary succeed.");
    }
    
    tuple_bs write_binary_cpx(const char* filepath, bool b_swap = true)
    {
        using namespace std;
        ofstream ofs(filepath, ofstream::binary);
        if(!ofs.is_open())
            return make_tuple(false,"ofs.open failed.");

        for(int i=0; i< width * height; i++){
            _Ty real = (b_swap ? binary_swap(array[i].real()) : array[i].real());
            _Ty imag = (b_swap ? binary_swap(array[i].imag()) : array[i].imag());
            ofs.write((char*)(&real),sizeof(_Ty));
            ofs.write((char*)(&imag),sizeof(_Ty));
        }

        ofs.close();
        return make_tuple(true,"write_binary succeed.");
    }

    tuple_bs read_tif_cpx(const char* filepath)
    {
        using namespace std;
        GDALAllRegister();
        GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(filepath,GA_ReadOnly));
        if(ds == nullptr){
            return make_tuple(false, "dataset is nullptr.");
        }
        if(ds->GetRasterXSize()!=width || ds->GetRasterYSize() != height){
            GDALClose(ds);
            return make_tuple(false, "ds.size is diff with width or height.");
        }
        
        GDALRasterBand* rb = ds->GetRasterBand(1);
        CPLErr err = rb->RasterIO(GF_Read,0,0,width,height,array,width,height,rb->GetRasterDataType(),0,0);
        GDALClose(ds);
        if(err == CE_Failure){
            return make_tuple(false,"rasterband.rasterio.read failed.");
        }
        return make_tuple(true,"read_tif succeed.");;
    }

    tuple_bs write_tif_cpx(const char* filepath, GDALDataType dt)
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
