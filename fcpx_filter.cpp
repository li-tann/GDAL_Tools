#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>

#define EXE_NAME "fcpx_filter"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum filter_method{unknown = 0, average_5, average_7};


int main(int argc, char* argv[])
{

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [method] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, fcpx filepath.\n"
                " argv[2]: method, 1:average_5, 2:average_7.\n"
                " argv[3]: output, fcpx filepath.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    
    filter_method method = filter_method(atoi(argv[2]));
    if(method < 1 || method > 2){
        return return_msg(-2,"method error.");
    }

    GDALAllRegister();

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(argv[1],GA_ReadOnly);
    if(!ds_in){
        return return_msg(-1, "ds_in is nullptr.");
    }
    GDALRasterBand* rb_in = ds_in->GetRasterBand(1);

    int width = ds_in->GetRasterXSize();
    int height = ds_in->GetRasterYSize();
    GDALDataType datatype = rb_in->GetRasterDataType();

    if(datatype != GDT_CFloat32){
        return return_msg(-2, "datatype isn't GDT_CFloat32.");
    }

    complex<float>* arr_in = new complex<float>[width * height];
    rb_in->RasterIO(GF_Read, 0, 0, width, height, arr_in, width, height, datatype, 0, 0);
    complex<float>* arr_out = new complex<float>[width * height];
    rb_in->RasterIO(GF_Read, 0, 0, width, height, arr_out, width, height, datatype, 0, 0);
    GDALClose(ds_in);

    int left = 2, right = 3;
    switch (method){
        case average_5:{left=2;right=3;}break;
        case average_7:{left=3;right=4;}break;
        }

    cout<<"percent:";
    int percent = 0;
    for(int i=0; i<height; i++)
    {
        if(i * 100 / height > percent){
            percent = i * 100 / height;
            cout<<"\rpercent: "<<percent<<"%";
        }
        if(i < left || i > height - right)
            continue;
        for(int j=0; j<width; j++)
        {
            if(j < left || j > width - right)
                continue;

            complex<double> sum(0,0);
            int num = 0;
            for(int row = i-left; row < i+right; row++){
                for(int col = j-left; col < j+right; col++){
                    sum += arr_in[row * width + col];
                    ++num;
                }
            }
            arr_out[i*width+j] = complex<float>(sum.real()/num, sum.imag()/num);
        }
    }
    cout<<endl;
    delete[] arr_in;

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = driver->Create(argv[3],width, height, 1, datatype, NULL);
    if(!ds_out){
        return return_msg(-3,"ds_out is nullptr");
    }

    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
    rb_out->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, datatype, 0, 0);
    delete[] arr_out;
    
    GDALClose(ds_out);
    
    return return_msg(1, EXE_NAME " end.");
}
