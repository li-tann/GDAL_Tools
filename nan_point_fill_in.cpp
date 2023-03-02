#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

using namespace std;

int main(int argc, char* argv[])
{
    GDALAllRegister();
    string msg;
    CPLErr err;

    auto my_logger = spdlog::basic_logger_mt("_NAN-Points_Fill_in_Val", "_NAN-Points_Fill_in_Val.txt");

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    if(argc < 4){
        msg =   "_NAN-Points_Fill_in_Val.exe\n"
                " manual:\n" 
                " argv[0]: _NAN-Points_Fill_in_Val,\n"
                " argv[1]: input imgpath, support datatype list: short, int, float, double\n"
                " argv[2]: output imgpath,\n"
                " argv[3]: value, which is NAN convert to.";
        return return_msg(-1,msg);
    }

    my_logger->info("_NAN-Points_Fill_in_Val start.");
    spdlog::info("_NAN-Points_Fill_in_Val start.");

    double val = atof(argv[3]);

    GDALDataset* ds_in = static_cast<GDALDataset*>(GDALOpen(argv[1],GA_ReadOnly));
    if(ds_in == nullptr)
        return return_msg(-2,"img_in.dataset is nullptr.");

    int xsize = ds_in->GetRasterXSize();
    int ysize = ds_in->GetRasterYSize();
    int bands = ds_in->GetRasterCount();
    GDALDataType datatype = ds_in->GetRasterBand(1)->GetRasterDataType();
    int sizeof_datatype = 0;

    switch(datatype){
        case GDT_Int16:{
            sizeof_datatype = sizeof(short);
        }break;
        case GDT_Int32:{
            sizeof_datatype = sizeof(int);
        }break;
        case GDT_Float32:{
            sizeof_datatype = sizeof(float);
        }break;
        case GDT_Float64:{
            sizeof_datatype = sizeof(double);
        }break;
        default:{
            return  return_msg(-3, "unsupported datatype(support list: short, int, float, double)");
        }
    }

    GDALDriver* pDriver = GetGDALDriverManager()->GetDriverByName(ds_in->GetDriverName());
    GDALDataset* ds_out = pDriver->Create(argv[2],xsize,ysize,bands,datatype,NULL);
    if(ds_out == nullptr)
        return  return_msg(-4, "img_out.dataset is nullptr.");

    err = ds_out->SetProjection(ds_in->GetProjectionRef());
    if(err > 1)
        return  return_msg(-5, "ds_out.setProjection(ds_in->GetProjectionRef()) failed.");

    double gt[6];
    err = ds_in->GetGeoTransform(gt);
    if(err > 1)
        return  return_msg(-6, "ds_in.GetGeoTransform()) failed.");

    err = ds_out->SetGeoTransform(gt);
    if(err > 1)
        return  return_msg(-6, "ds_out.SetGeoTransform()) failed.");

    for(int band = 1; band < bands; band++)
    {
        GDALRasterBand* rb_in = ds_in->GetRasterBand(band);
            GDALRasterBand* rb_out = ds_out->GetRasterBand(band);
        for(int i=0; i< ysize; i++)
        {
            switch (datatype)
            {
            case GDT_Int16:{
                short* arr = new short[xsize];
                err = rb_in->RasterIO(GF_Read,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(read) short failed.");

                for(int j=0; j<xsize; j++)
                    if(arr[j] == -32767)arr[j] = val;
                rb_out->RasterIO(GF_Write,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(write) short failed.");

                delete []arr;
            }break;
            case GDT_Int32:{
                int* arr = new int[xsize];
                err = rb_in->RasterIO(GF_Read,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(read) int failed.");

                for(int j=0; j<xsize; j++)
                    if(arr[j] == -32767)arr[j] = val;
                err = rb_out->RasterIO(GF_Write,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(write) int failed.");

                delete []arr;
            }break;
            case GDT_Float32:{
                float* arr = new float[xsize];
                err = rb_in->RasterIO(GF_Read,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(read) float failed.");

                for(int j=0; j<xsize; j++)
                    if(isnan(arr[j]))arr[j] = val;
                err = rb_out->RasterIO(GF_Write,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(write) float failed.");

                delete []arr;
            }break;
            case GDT_Float64:{
                double* arr = new double[xsize];
                err = rb_in->RasterIO(GF_Read,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(read) double failed.");

                for(int j=0; j<xsize; j++)
                    if(isnan(arr[j]))arr[j] = val;
                err = rb_out->RasterIO(GF_Write,0,i,xsize,1,arr,xsize,1,datatype,0,0);
                if(err > 1)
                   return  return_msg(-7, "rb_in.rasterio(write) double failed.");

                delete []arr;
            }break;
            default:
                break;
            }
        }
    }

    GDALClose(ds_in);
    GDALClose(ds_out);

    return return_msg(1, "_NAN-Points_Fill_in_Val end.");

}


    