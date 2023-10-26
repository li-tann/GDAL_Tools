#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>

#include "datatype.h"

#define EXE_NAME "_vrt_trans"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum class method{unknown, point, txt, dem};

using namespace std;

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
        msg +=  " manual: " EXE_NAME " [method] [filepath]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: method , v2t or t2v.\n"
                " argv[2]: input  , vrt or tif filepath.\n"
                " argv[3]: output , tif or bin filepath.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    bool b_v2t;
    if(string(argv[1]) == "v2t"){
        b_v2t = true;
    }else if(string(argv[1]) == "t2v"){
        b_v2t = false;
    }else{
        msg = "method is not both 'v2t' & 't2v'";
        return return_msg(-1,msg);
    }
 
    GDALAllRegister();

    if(b_v2t){

        /// vrt 2 tif

        std::string input_filepath = argv[2];
        fs::path path_input(input_filepath);
        std::string binary_name = path_input.filename().string();
        std::string binary_path = input_filepath;
        std::string vrt_path = binary_path + ".vrt";
        
        GDALDataset* ds_in = (GDALDataset*)GDALOpen(argv[2],GA_ReadOnly);
        if(!ds_in){
            return return_msg(-2,"ds_in is nullptr");
        }
        GDALRasterBand* rb_in = ds_in->GetRasterBand(1);

        int width = ds_in->GetRasterXSize();
        int height = ds_in->GetRasterYSize();
        GDALDataType datatype = rb_in->GetRasterDataType();

        GDALDriver* driver_tif = GetGDALDriverManager()->GetDriverByName("GTiff");
        GDALDataset* ds_out = driver_tif->Create(argv[3],width, height,1,datatype,NULL);
        GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
        switch (datatype)
        {
        case GDT_CFloat32:{
            std::complex<float>* arr = new complex<float>[width];
            int percentage = 0;
            std::cout<<"percent: ";
            for(int i=0; i<height; i++){
                if(i * 100 / height > percentage){
                    percentage = i * 100 / height;
                    std::cout<<"\rpercent: "<<percentage+1 <<"%("<<i<<"/"<<height<<")";
                }
                rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width,1,GDT_CFloat32,0,0);
                rb_out->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, GDT_CFloat32, 0, 0);
            }
            std::cout<<std::endl;
            delete[] arr;
        }break;
        case GDT_Float32:{
            float* arr = new float[width];
            int percentage = 0;
            std::cout<<"percent: ";
            for(int i=0; i<height; i++){
                if(i * 100 / height > percentage){
                    percentage = i * 100 / height;
                    std::cout<<"\rpercent: "<<percentage+1 <<"%("<<i<<"/"<<height<<")";
                }
                rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width,1,GDT_Float32,0,0);
                rb_out->RasterIO(GF_Write, 0, i, width, 1, arr, width, 1, GDT_Float32, 0, 0);
            }
            std::cout<<std::endl;
            delete[] arr;
        }break;
        default:{
            std::cout<<"unknown datatype.\n";
            return 1;
        }
        }

        GDALClose(ds_in);
        GDALClose(ds_out);
    }
    else{  

        /// tif 2 vrt

        fs::path path_bin(argv[3]);
        std::string binary_name = path_bin.filename().string();
        std::string vrt_path = path_bin.string() + ".vrt";

        GDALDataset* ds_in = (GDALDataset*)GDALOpen(argv[2],GA_ReadOnly);
        if(!ds_in){
            return return_msg(-2,"ds_in is nullptr");
        }
        GDALRasterBand* rb_in = ds_in->GetRasterBand(1);

        int width = ds_in->GetRasterXSize();
        int height = ds_in->GetRasterYSize();
        GDALDataType datatype = rb_in->GetRasterDataType();


        std::ofstream ofs(argv[3],ios::binary);
        if(!ofs.is_open()){
            return return_msg(-3,"ofs open failed.");
        }


        int PixelOffset = 0;
        int LineOffset = 0;
        switch (datatype)
        {
        case GDT_CFloat32:{
            PixelOffset = 8;
            LineOffset = PixelOffset * width;
            std::complex<float>* arr = new complex<float>[width];
            int percentage = 0;
            std::cout<<"percent: ";
            for(int i=0; i<height; i++){
                if(i * 100 / height > percentage){
                    percentage = i * 100 / height;
                    std::cout<<"\rpercent: "<<percentage+1 <<"%("<<i<<"/"<<height<<")";
                }
                rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width,1,GDT_CFloat32,0,0);
                float* arr_flt = (float*)(arr);
                for (int i = 0; i < 2 * width; i++) {
                    float val = swap(arr_flt[i]);
                    ofs.write((char*)(&val), sizeof(val));
                }
            }
            std::cout<<std::endl;
            delete[] arr;
        }break;
        case GDT_Float32:{
            PixelOffset = 4;
            LineOffset = PixelOffset * width;
            float* arr = new float[width];
            int percentage = 0;
            std::cout<<"percent: ";
            for(int i=0; i<height; i++){
                if(i * 100 / height > percentage){
                    percentage = i * 100 / height;
                    std::cout<<"\rpercent: "<<percentage+1 <<"%("<<i<<"/"<<height<<")";
                }
                rb_in->RasterIO(GF_Read, 0, i, width, 1, arr, width,1,GDT_Float32,0,0);
                for (int i = 0; i < width; i++) {
                    float val = swap(arr[i]);
                    ofs.write((char*)(&val), sizeof(val));
                }
            }
            std::cout<<std::endl;
            delete[] arr;
        }break;
        default:{
            return return_msg(-3,"unsupport datatype");
        }
        }

        ofs.close();


        GDALDriver* driver_vrt = GetGDALDriverManager()->GetDriverByName("VRT");
        GDALDataset* ds_out = driver_vrt->Create(vrt_path.c_str(), width, height, 0,GDT_Float32,NULL);
        if(!ds_out){
            return return_msg(-2,"ds_out is nullptr");
        }
        char** papszOptions = NULL;
        papszOptions = CSLAddNameValue(papszOptions, "subclass", "VRTRawRasterBand"); // if not specified, default to VRTRasterBand
        papszOptions = CSLAddNameValue(papszOptions, "SourceFilename", binary_name.c_str()); // mandatory
        papszOptions = CSLAddNameValue(papszOptions, "ImageOffset", "0"); // optional. default = 0
        papszOptions = CSLAddNameValue(papszOptions, "PixelOffset", to_string(PixelOffset).c_str()); // optional. default = size of band type
        papszOptions = CSLAddNameValue(papszOptions, "LineOffset", to_string(LineOffset).c_str()); // optional. default = size of band type * width
        papszOptions = CSLAddNameValue(papszOptions, "ByteOrder", "MSB"); // optional. default = machine order
        papszOptions = CSLAddNameValue(papszOptions, "relativeToVRT", "true"); // optional. default = false
        ds_out->AddBand(datatype, papszOptions);
        CSLDestroy(papszOptions);

        delete ds_out;


        
    }
    

    return return_msg(1, EXE_NAME " end.");
}
