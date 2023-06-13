#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>

#define EXE_NAME "_vrt_io"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum class method{unknown, point, txt, dem};


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

   
    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [method] [filepath]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: method, read or write.\n"
                " argv[2]: vrt filepath, input or output.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    bool b_read;
    if(string(argv[1]) == "read"){
        b_read = true;
    }else if(string(argv[1]) == "write"){
        b_read = false;
    }else{
        msg = "method is not both 'read' & 'write'";
        return return_msg(-1,msg);
    }

    std::string filepath = argv[2];
    fs::path file_p(filepath);
    std::string binary_name = file_p.filename().string();
    std::string binary_path = filepath;
    std::string vrt_path = binary_path + ".vrt";
    if(!b_read){

    }

    GDALAllRegister();

    if(b_read){

        GDALDataset* ds_in = (GDALDataset*)GDALOpen(argv[2],GA_ReadOnly);
        if(!ds_in){
            return return_msg(-2,"ds_in is nullptr");
        }
        GDALRasterBand* rb_in = ds_in->GetRasterBand(1);

        return_msg(0,fmt::format("width:    [{}]",ds_in->GetRasterXSize()));
        return_msg(0,fmt::format("height:   [{}]",ds_in->GetRasterYSize()));
        return_msg(0,fmt::format("bands:    [{}]",ds_in->GetRasterCount()));
        return_msg(0,fmt::format("datatype: [{}]",rb_in->GetRasterDataType()));

        float* value = new float[1];
        rb_in->RasterIO(GF_Read,2,2,1,1,value,1,1,GDT_Float32,0,0);
        return_msg(0,fmt::format("value(2,2): [{}]",*value));

        GDALClose(ds_in);
    }else{  /// write

        int width = 1024;
        int height = 512;
        float* arr = new float[height*width];

        std::ofstream ofs(binary_path,ios::binary);
        if(!ofs.is_open()){
            return return_msg(-3,"ofs open failed.");
        }

        for(int i=0; i<height*width;i++){
            int row = i / width;
            int col = i % width;
            arr[i] = float(row + col);
        }
        ofs.write((const char*)arr,height*width*4);
        ofs.close();
        delete[] arr;

        // GDALDriver* driver_mem = GetGDALDriverManager()->GetDriverByName("MEM");
        // GDALDriver* driver_vrt = GetGDALDriverManager()->GetDriverByName("VRT");

        // GDALDataset* ds_mem = driver_mem->Create("",1000,1000,1,GDT_Float32,nullptr);
        // if(!ds_mem){
        //     return return_msg(-2,"ds_mem is nullptr");
        // }
        // ds_mem->GetRasterBand(1)->RasterIO(GF_Write,0,0,1000,1000,arr,1000,1000,GDT_Float32,0,0);
        // delete[] arr;

        // GDALDataset* ds_out = driver_vrt->CreateCopy(argv[2],ds_mem,FALSE,NULL,NULL,NULL);
        // if(!ds_out){
        //     return return_msg(-2,"ds_out is nullptr");
        // }
        // GDALClose((GDALDatasetH) ds_out);
        // GDALClose((GDALDatasetH) ds_mem);

        GDALDriver* driver_vrt = GetGDALDriverManager()->GetDriverByName("VRT");
        GDALDataset* ds_out = driver_vrt->Create(vrt_path.c_str(), width, height, 0,GDT_Float32,NULL);
        if(!ds_out){
            return return_msg(-2,"ds_out is nullptr");
        }
        char** papszOptions = NULL;
        papszOptions = CSLAddNameValue(papszOptions, "subclass", "VRTRawRasterBand"); // if not specified, default to VRTRasterBand
        papszOptions = CSLAddNameValue(papszOptions, "SourceFilename", binary_name.c_str()); // mandatory
        papszOptions = CSLAddNameValue(papszOptions, "ImageOffset", "0"); // optional. default = 0
        papszOptions = CSLAddNameValue(papszOptions, "PixelOffset", "4"); // optional. default = size of band type
        papszOptions = CSLAddNameValue(papszOptions, "LineOffset", to_string(width*4).c_str()); // optional. default = size of band type * width
        papszOptions = CSLAddNameValue(papszOptions, "ByteOrder", "LSB"); // optional. default = machine order
        papszOptions = CSLAddNameValue(papszOptions, "relativeToVRT", "true"); // optional. default = false
        ds_out->AddBand(GDT_Float32, papszOptions);
        CSLDestroy(papszOptions);

        delete ds_out;


        
    }
    

    return return_msg(1, EXE_NAME " end.");
}
