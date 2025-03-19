#include <iostream>
#include <fstream>
#include <filesystem>

#include <half.hpp>
#include <fmt/format.h>
#include <gdal_priv.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <argparse/argparse.hpp>

#include "datatype.h"

#ifndef PRINT_LOGGER
#define PRINT_LOGGER(LOGGER, TYPE, MASSAGE)  \
    LOGGER->TYPE(MASSAGE);                   \
    spdlog::TYPE(MASSAGE);                   
#endif

namespace fs = std::filesystem;
using std::cout, std::cin, std::endl, std::string, std::vector, std::map;

int save_float32_to_float16_binary(float* flt_arr, size_t size, const char* binary_path);
float* load_float16_binary_to_float32(const char* binary_path, size_t size);

int float32_tif_to_float16_binary(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);
int float16_binary_to_float32_tif(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int save_tif_info_as_vrt(GDALDataset* ds, const char* vrt);
int load_tif_info_from_vrt(const char* vrt, int &height, int &width, double* gt, std::string &proj);

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("float32_16", "1.0", argparse::default_arguments::help);
    program.add_description("translate between float32(tif) & float16(binary)");

    argparse::ArgumentParser sub_32_to_16("float32_to_16");
    sub_32_to_16.add_description("stored the float32 tif to float16 binary (& info_text).");
    {
        sub_32_to_16.add_argument("float32_tif")
            .help("tif filepath with float datatype.");

        sub_32_to_16.add_argument("float16_bin")
            .help("binary filepath to stored float16 data");

        sub_32_to_16.add_argument("float16_info")
            .help("text filepath to stored info of float16 data, like: width, and height");
    }
    
    
    argparse::ArgumentParser sub_16_to_32("float16_to_32");
    sub_16_to_32.add_description("stored the float16 binary (& info_text) to float32 tif.");
    {
        sub_16_to_32.add_argument("float16_bin")
            .help("binary filepath, which stored float16 data");

        sub_16_to_32.add_argument("float16_info")
            .help("text filepath, which stored info of float16 data");

        sub_16_to_32.add_argument("float32_tif")
            .help("tif filepath to stored float32 data.");
    }


    std::map<argparse::ArgumentParser* , 
            std::function<int(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger>)>> 
    parser_map_func = {
        {&sub_32_to_16,   float32_tif_to_float16_binary},
        {&sub_16_to_32,   float16_binary_to_float32_tif},

    };

    for(auto prog_map : parser_map_func){
        program.add_subparser(*(prog_map.first));
    }

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl<<std::endl;
        for(auto prog_map : parser_map_func){
            if(program.is_subcommand_used(*(prog_map.first))){
                std::cerr << *(prog_map.first) <<std::endl;
                return 1;
            }
        }
        std::cerr << program;
        return 1;
    }

    /// log
    char* pgmptr = 0;
    _get_pgmptr(&pgmptr);
    fs::path exe_root(fs::path(pgmptr).parent_path());
    fs::path log_path = exe_root / "read_egm2008.log";
    auto my_logger = spdlog::basic_logger_mt("read_egm2008", log_path.string());

    auto start = chrono::system_clock::now();
    string msg;

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    std::string config;
    for(int i=0; i<argc; i++){
        config += std::string(argv[i]) + " ";
    }
    PRINT_LOGGER(my_logger, info, "read_egm2008 start");
    PRINT_LOGGER(my_logger, info, fmt::format("config:[{}]",config));
    auto time_start = std::chrono::system_clock::now();

    for(auto& iter : parser_map_func){
        if(program.is_subcommand_used(*(iter.first))){
            return iter.second(iter.first, my_logger);
        } 
    }
    PRINT_LOGGER(my_logger, info, fmt::format("read_egm2008 end, spend time {}s",spend_time(time_start)));
    return 1;






    // half_float::half h_val;
    // h_val = 0.1234567f;
    
    // float flt_arr[4] = {0.1234567f, 0.4567432f, -1.78642f, 2.235322f};
    // std::cout<<"flt_arr:"<<std::endl;
    // for(int i=0; i<4; i++){
    //     std::cout<<flt_arr[i]<<",";
    // }
    // std::cout<<std::endl;

    // half_float::half half_arr[4];
    // for(int i=0; i<4; i++){
    //     half_arr[i] = half_float::half_cast<half_float::half, float>(flt_arr[i]);
    // }

    // std::cout<<"half_arr:"<<std::endl;
    // for(int i=0; i<4; i++){
    //     std::cout<<half_arr[i]<<",";
    // }
    // std::cout<<std::endl;

    
    // // std::frexp()

    // std::ofstream ofs("half.bin", std::ios::binary);
    // ofs.write(reinterpret_cast<const char*>(half_arr), sizeof(half_float::half) * 4);
    // ofs.close();


    // half_float::half half_arr_2[4];
    // std::ifstream ifs("half.bin", std::ios::binary);
    // ifs.read(reinterpret_cast<char*>(&half_arr_2), sizeof(half_float::half) * 4);
    // ifs.close();

    // std::cout<<"half_arr_2:"<<std::endl;
    // for(int i=0; i<4; i++){
    //     std::cout<<half_arr_2[i]<<",";
    // }
    // std::cout<<std::endl;







//    size_t len = 512;
//     float* arr = new float[len];
//     for(int i=0; i<len; i++){
//         arr[i] = i + float(i)/len;
//     }

//     auto rst = save_float32_to_float16_binary(arr, len, "arr_half.binary");
//     std::cout<<"rst:"<<rst<<std::endl;

//     float* arr_new = load_float16_binary_to_float32("arr_half.binary", len);
//     if(!arr_new){
//         std::cout<<"arr_new is nullptr"<<std::endl;
//         return -1;
//     }

//     for(int i=0; i<len; i++){
//         // std::cout<<fmt::format("{:10.f}, {:10.f}", arr[i], arr_new[i])<<std::endl;
//         std::cout<<fmt::format("{}, {}", arr[i], arr_new[i])<<std::endl;
//     }


//     delete[] arr;
//     delete[] arr_new;




    return 1;
}

int save_float32_to_float16_binary(float* flt_arr, size_t size, const char* binary_path)
{
    std::ofstream ofs(binary_path, std::ios::binary);
    if(!ofs.is_open()){
        spdlog::error("open ofs(float16 binary) failed...");
        return 1;
    }
    half_float::half hf;
    for(size_t i=0; i<size; i++){
        hf = flt_arr[i];
        ofs.write(reinterpret_cast<const char*>(&hf), sizeof(half_float::half));
    }
    ofs.close();
    return 0;
}

float* load_float16_binary_to_float32(const char* binary_path, size_t size)
{
    std::ifstream ifs(binary_path, std::ios::binary);
    if(!ifs.is_open()){
        spdlog::error("open float16(binary) failed...");
        return nullptr;
    }
    float* arr = new (std::nothrow)float[size];
    if(!arr){
        spdlog::error("malloc(new) arr failed...");
        return nullptr;
    }
    half_float::half hf;
    for(size_t i=0; i<size; i++){
        ifs.read(reinterpret_cast<char*>(&hf), sizeof(half_float::half));
        arr[i] = hf;
    }
    ifs.close();

    return arr;
}

int float32_tif_to_float16_binary(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    /*
    sub_32_to_16.add_argument("float32_tif")
    sub_32_to_16.add_argument("float16_bin")
    sub_32_to_16.add_argument("float16_info")
    */

    std::string float32_path = args->get<std::string>("float32_tif");
    std::string float16_path = args->get<std::string>("float16_bin");
    std::string float16_info_text = args->get<std::string>("float16_info");

    PRINT_LOGGER(logger, info, fmt::format("float32_tif:  '{}'", float32_path));
    PRINT_LOGGER(logger, info, fmt::format("float16_bin:  '{}'", float16_path));
    PRINT_LOGGER(logger, info, fmt::format("float16_info: '{}'", float16_info_text));

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    spdlog::info("load float32(tif)...");

    GDALDataset* ds = (GDALDataset*)GDALOpen(float32_path.c_str(), GA_ReadOnly);
    if(!ds){
        spdlog::error("ds(float32) is nullptr.");
        return 1;
    }

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    auto datatype = ds->GetRasterBand(1)->GetRasterDataType();

    if(datatype != GDT_Float32){
        spdlog::error("datatype(float32) is not gdt_float32.");
        GDALClose(ds);
        return 2;
    }

    float* arr = new (std::nothrow)float[width * height];
    if(!arr){
        spdlog::error("arr(float32) is nullptr.");
        GDALClose(ds);
        return 3;
    }

    ds->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);

    spdlog::info("save float32(arr) to float16(binary).");

    auto rst = save_float32_to_float16_binary(arr, width*height, float16_path.c_str());
    if(rst != 0){
        spdlog::error("ofs(float16) open failed.");
        GDALClose(ds);
        delete[] arr;
        return 4;
    }
    delete[] arr;

    spdlog::info("save float32(tif) info as vrt.");

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("VRT");
    GDALDataset* ds_vrt = driver->CreateCopy(float16_info_text.c_str(), ds, false, nullptr, nullptr, nullptr);
    if(!ds_vrt){
        GDALClose(ds);
        spdlog::error("ds_vrt is nullptr.");
        return 5;
    }

    GDALClose(ds_vrt);
    GDALClose(ds);
    return 0;
}

int float16_binary_to_float32_tif(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    /*
    sub_32_to_16.add_argument("float32_tif")
    sub_32_to_16.add_argument("float16_bin")
    sub_32_to_16.add_argument("float16_info")
    */

    std::string float32_path = args->get<std::string>("float32_tif");
    std::string float16_path = args->get<std::string>("float16_bin");
    std::string float16_info_text = args->get<std::string>("float16_info");

    PRINT_LOGGER(logger, info, fmt::format("float32_tif:  '{}'", float32_path));
    PRINT_LOGGER(logger, info, fmt::format("float16_bin:  '{}'", float16_path));
    PRINT_LOGGER(logger, info, fmt::format("float16_info: '{}'", float16_info_text));

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    spdlog::info("load float16 info from 'float16_info_text'.");

    GDALDataset* ds_vrt = (GDALDataset*)GDALOpen(float16_info_text.c_str(), GA_ReadOnly);
    if(!ds_vrt){
        spdlog::error("ds_vrt is nullptr.");
        return 1;
    }
    int width = ds_vrt->GetRasterXSize();
    int height= ds_vrt->GetRasterYSize();

    spdlog::info(fmt::format("width: {}", width));
    spdlog::info(fmt::format("height:{}", height));

    if(width <= 0 || height <= 0){
        spdlog::error("there is no 'width' or 'height' in ifs(float16 info).");
        GDALClose(ds_vrt);
        return 2;
    }

    float* float32_arr = load_float16_binary_to_float32(float16_path.c_str(), width*height);
    if(!float32_arr){
        GDALClose(ds_vrt);
        spdlog::error("float32_arr is nullptr.");
        return 3;
    }

    spdlog::info("create dataset & rasterio...");

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds = driver->Create(float32_path.c_str(), width, height, 1, GDT_Float32, NULL);
    if(!ds){
        delete[] float32_arr;
        GDALClose(ds_vrt);
        spdlog::error("ds is nullptr.");
        return 4;
    }

    ds->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, width, height, float32_arr, width, height, GDT_Float32, 0, 0);

    /// @note 赋值
    double gt[6];
    if(ds_vrt->GetGeoTransform(gt) == CE_None){
        spdlog::info(fmt::format("set geoTransform: [{:.6f}, {:.6f}, {:.6f}, {:.6f}, {:.6f}, {:.6f}]",gt[0],gt[1],gt[2],gt[3],gt[4],gt[5]));
        ds->SetGeoTransform(gt);
    }

    spdlog::info(fmt::format("set projection..."));
    ds->SetProjection(ds_vrt->GetProjectionRef());

    int nodata_value_success;
    double nodata_value = ds_vrt->GetRasterBand(1)->GetNoDataValue(&nodata_value_success);
    if(nodata_value_success){
        spdlog::info(fmt::format("set SetNoDataValue:{}",nodata_value));
        ds->GetRasterBand(1)->SetNoDataValue(nodata_value);
    }
    
    GDALClose(ds_vrt);
    GDALClose(ds);
    delete[] float32_arr;


    return 0;

}


int load_tif_info_from_vrt(const char* vrt, int &height, int &width, double* gt, std::string &proj)
{
    GDALAllRegister();
    GDALDataset* ds = (GDALDataset*)GDALOpen(vrt, GA_ReadOnly);
    if(!ds){
        return 1;
    }

    height = ds->GetRasterYSize();
    width = ds->GetRasterXSize();

    double tmp_gt[6];
    ds->GetGeoTransform(tmp_gt);
    for(int i=0; i<6; i++){
        gt[i] = tmp_gt[i];
    }

    proj = ds->GetProjectionRef();

    GDALClose(ds);
    return 0;
}