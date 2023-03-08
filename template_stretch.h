#ifndef TEMPLATE_STRETCH
#define TEMPLATE_STRETCH

#include <gdal_priv.h>

#include <vector>
#include <string>
#include <tuple>

using tuple_bs = std::tuple<bool, std::string>;

template<typename _Ty>
tuple_bs histogram_stretch(GDALRasterBand* rb, int xsize, int ysize, _Ty min, _Ty max, GDALDataType datatype)
{
    _Ty* arr = new _Ty[xsize *ysize];
    rb->RasterIO(GF_Read, 0, 0, xsize, ysize, arr, xsize, ysize,datatype,0,0);
    for(int i=0; i < xsize * ysize; i++){
        if(arr[i] < min){arr[i] = min;}
        else if(arr[i] > max){arr[i] = max;}
    }
    rb->RasterIO(GF_Write, 0, 0, xsize, ysize, arr, xsize, ysize,datatype,0,0);
    delete[] arr;
}

template<typename _Ty>
tuple_bs rasterband_histogram_stretch(GDALRasterBand* rb, int band, int xsize, int ysize, int HISTOGRAM_SIZE, double stretch_rate)
{
    using namespace std;
    GDALDataType datatype = rb->GetRasterDataType();
    CPLErr err;
    double minmax[2];
    GUIntBig* histogram_result = new GUIntBig[HISTOGRAM_SIZE];
    err = rb->ComputeRasterMinMax(FALSE,minmax);
    if(err == CE_Failure){
        return make_tuple(false,"band " + to_string(band) + ".computeRasterMinMax failed.");
    }
    err = rb->GetHistogram(minmax[0], minmax[1],HISTOGRAM_SIZE, histogram_result, FALSE, FALSE, GDALDummyProgress, nullptr);
    if(err == CE_Failure){
        return make_tuple(false ,"band " + to_string(band) + ".getHistrogram failed.");
    }


    /// statistics, accumulate & accumulate_percent
    GUIntBig* histogram_accumulate = new GUIntBig[HISTOGRAM_SIZE];
    histogram_accumulate[0] = histogram_result[0];
    for(int i=1; i<HISTOGRAM_SIZE; i++){
        histogram_accumulate[i] = histogram_accumulate[i-1] + histogram_result[i];
    }

    double* histogram_accumulate_percent = new double[HISTOGRAM_SIZE];
    double min, max;
    for(int i=0; i<HISTOGRAM_SIZE; i++){
        histogram_accumulate_percent[i] = 1. * histogram_accumulate[i] / histogram_accumulate[HISTOGRAM_SIZE-1] ;
        if(i==0)continue;
        if((histogram_accumulate_percent[i-1] <= stretch_rate || i == 1) && histogram_accumulate_percent[i] >= stretch_rate){
            min = minmax[0] + (minmax[1] - minmax[0])/ HISTOGRAM_SIZE * (i-1);
        }
        if(histogram_accumulate_percent[i-1] <= 1 - stretch_rate && histogram_accumulate_percent[i] >= 1 - stretch_rate){
            max = minmax[0] + (minmax[1] - minmax[0])/ HISTOGRAM_SIZE * i;
        }
    }
    delete[] histogram_accumulate;
    delete[] histogram_accumulate_percent;

    _Ty _TyMin = _Ty(min);
    _Ty _TyMax = _Ty(max);

    _Ty* arr = new _Ty[xsize *ysize];
    rb->RasterIO(GF_Read, 0, 0, xsize, ysize, arr, xsize, ysize,datatype,0,0);
    for(int i=0; i < xsize * ysize; i++){
        if(arr[i] < _TyMin){arr[i] = _TyMin;}
        else if(arr[i] > _TyMax){arr[i] = _TyMax;}
    }
    rb->RasterIO(GF_Write, 0, 0, xsize, ysize, arr, xsize, ysize,datatype,0,0);
    delete[] arr;

    return make_tuple(true,"band " + to_string(band) + ".computeRasterMinMax failed.");
}



#endif ///TEMPLATE_STRETCH