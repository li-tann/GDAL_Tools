#ifndef TEMPLATE_NAN_CONVERT_TO
#define TEMPLATE_NAN_CONVERT_TO

#include <gdal_priv.h>

#include <vector>
#include <string>
#include <tuple>

using tuple_bs = std::tuple<bool, std::string>;


/// gdaldataset* ds must be opened by gdalopen(..., ga_update)
template<typename _Ty>
tuple_bs nodata_transto(GDALDataset* ds, _Ty value)
{
    using namespace std;
    string func_name = "nodata_transto";
    cout <<func_name <<" start."<<endl;
    
    if(ds == nullptr)
        return make_tuple(false, "ds is nullptr");

    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();

    cout<<"progress: ";
    double last_percent = -1;
    for(int b = 1;b <= bands; b++)
    {
        GDALRasterBand* rb = ds->GetRasterBand(b);
        for(int y = 0; y< ysize; y++)
        {
            _Ty* arr = new _Ty[xsize];
            
            rb->RasterIO(GF_Read,0,y,xsize,1,arr,xsize,1,rb->GetRasterDataType(),0,0);
            for(int x=0; x< xsize; x++){
                if(isnan(arr[x])){arr[x] = value;}
            }
            rb->RasterIO(GF_Write,0,y,xsize,1,arr,xsize,1,rb->GetRasterDataType(),0,0);
            delete[] arr;

            ///print prog,
            double percent = double((b-1) *ysize + y) / (bands * ysize) ;
            if(percent - last_percent > 0.01)
            {
                last_percent = percent;
                if(int(percent * 100)%5 != 0)
                    continue;

                if(int(percent * 100)%10 == 0)
                    cout<< int(percent * 100);
                else
                    cout<< "..";
            }
        }
    }
    cout<<100<<endl;

    cout<<func_name <<" successd."<<endl;
    return make_tuple(true, "nodata_transto func successd.");
}


template<typename _Ty>
tuple_bs value_transto(GDALDataset* ds, _Ty value_in, _Ty value_out)
{
    using namespace std;
    string func_name = "value_transto";
    cout <<func_name <<" start."<<endl;

    if(ds == nullptr)
        return make_tuple(false, "ds is nullptr");

    int xsize = ds->GetRasterXSize();
    int ysize = ds->GetRasterYSize();
    int bands = ds->GetRasterCount();

    cout<<"progress: ";
    double last_percent = -1;
    for(int b = 1;b <= bands; b++)
    {
        GDALRasterBand* rb = ds->GetRasterBand(b);
        for(int y = 0; y< ysize; y++)
        {
            _Ty* arr = new _Ty[xsize];
            rb->RasterIO(GF_Read,0,y,xsize,1,arr,xsize,1,rb->GetRasterDataType(),0,0);
            for(int x=0; x< xsize; x++){
                if(arr[x] == value_in){arr[x] = value_out;}
            }
            rb->RasterIO(GF_Write,0,y,xsize,1,arr,xsize,1,rb->GetRasterDataType(),0,0);
            delete[] arr;

            ///print prog,
            double percent = double((b-1) *ysize + y) / (bands * ysize) ;
            if(percent - last_percent > 0.01)
            {
                last_percent = percent;
                if(int(percent * 100)%5 != 0)
                    continue;

                if(int(percent * 100)%10 == 0)
                    cout<< int(percent * 100);
                else
                    cout<< "..";
            }
        }
    }
    cout<<100<<endl;

    cout<<func_name <<" successd."<<endl;
    return make_tuple(true, "value_transto func successd.");
}

#endif //TEMPLATE_NAN_CONVERT_TO