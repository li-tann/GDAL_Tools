#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <regex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fmt/format.h>

#include <gdal_priv.h>

#include "datatype.h"

#define EXE_NAME "geoimage_copy_ref_paste_to_target"

#define DEBUG

// #define PRINT_DETAILS
// #define SYSTEM_PAUSE

using namespace std;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

int main(int argc, char* argv[])
{
    // argc = 7;
    // argv = new char*[7];
    // for(int i=0; i<7; i++){
    //     argv[i] = new char[256];
    // }
    // strcpy(argv[1], "E:\\DEM");
    // // strcpy(argv[1], "D:\\1_Data\\shp_test\\TanDEM_DEM");
    // strcpy(argv[2], "D:\\1_Data\\china_shp\\bou1_4p.shp");
    // strcpy(argv[3], "0");
    // strcpy(argv[4], "0.1");
    // strcpy(argv[5], ".*DEM.tif");
    // strcpy(argv[6], "D:\\Copernicus_China_DEM_minimum.tif");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    /// 获取目标数据(target)的范围(start.x/y & width,height), 从参考数据(ref)中提取对应信息, 并更新到目标数据(target)上
    if(argc < 7){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [target filepath] [ref filepath] [target start x] [target start y] [target width] [target height]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input/output, target filepath.\n"
                " argv[2]: input, reference filepath.\n"
                " argv[3]: input, target start x( pixel). (int)\n"
                " argv[4]: input, target start y( pixel). (int)\n"
                " argv[5]: input, target width. (int)\n"
                " argv[6]: input, target height. (int)\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    int tar_start_x     = stoi(argv[3]);
    int tar_start_y     = stoi(argv[4]);
    int tar_block_width = stoi(argv[5]);
    int tar_block_height= stoi(argv[6]);


    GDALDataset* tar_ds = (GDALDataset*)GDALOpen(argv[1],GA_Update);
    if(!tar_ds){
        return return_msg(-1, "tar_ds is nullptr.");
    }
    int tar_width = tar_ds->GetRasterXSize();
    int tar_height= tar_ds->GetRasterYSize();
    double tar_gt[6];
    tar_ds->GetGeoTransform(tar_gt);
    GDALRasterBand* tar_rb = tar_ds->GetRasterBand(1);
    GDALDataType tar_datatype = tar_rb->GetRasterDataType();

    GDALDataset* ref_ds = (GDALDataset*)GDALOpen(argv[2],GA_Update);
    if(!ref_ds){
        GDALClose(tar_ds);
        return return_msg(-1, "ref_ds is nullptr.");
    }
    int ref_width = ref_ds->GetRasterXSize();
    int ref_height= ref_ds->GetRasterYSize();
    double ref_gt[6];
    ref_ds->GetGeoTransform(ref_gt);
    GDALRasterBand* ref_rb = ref_ds->GetRasterBand(1);
    GDALDataType ref_datatype = ref_rb->GetRasterDataType();

    if(ref_datatype != tar_datatype){
        return return_msg(-2, fmt::format("ref_dt({}) is diff width tar_dt({}).",
                                            GDALGetDataTypeName(ref_datatype), GDALGetDataTypeName(tar_datatype)
                                        )
                        );
    }

    int datatype_size = GDALGetDataTypeSize(ref_datatype);

#ifdef DEBUG
    spdlog::info(fmt::format("tar_gt:{},{},{},{},{},{}",tar_gt[0],tar_gt[1],tar_gt[2],tar_gt[3],tar_gt[4],tar_gt[5]));
    spdlog::info(fmt::format("ref_gt:{},{},{},{},{},{}",ref_gt[0],ref_gt[1],ref_gt[2],ref_gt[3],ref_gt[4],ref_gt[5]));
#endif

    int ref_start_x = (int)round((tar_gt[0] + tar_start_x * tar_gt[1] - ref_gt[0]) / ref_gt[1]);
    int ref_start_y = (int)round((tar_gt[3] + tar_start_y * tar_gt[5] - ref_gt[3]) / ref_gt[5]);

#ifdef DEBUG
    spdlog::info(fmt::format("ref_start_x:{}, ref_start_y:{}",ref_start_x, ref_start_y));
#endif

    if(ref_start_x < 0 || ref_start_x > ref_width - 1){
        return return_msg(-2, fmt::format("ref_start_x:{} error, ref range:[0, {}]",ref_start_x, ref_width - 1));
    }

    if(ref_start_y < 0 || ref_start_y > ref_height - 1){
        return return_msg(-2, fmt::format("ref_start_y:{} error, ref range:[0, {}]",ref_start_y, ref_height - 1));
    }

    int ref_block_width   = (int)round(tar_block_width  * tar_gt[1] / ref_gt[1]);
    int ref_block_height  = (int)round(tar_block_height * tar_gt[5] / ref_gt[5]);

#ifdef DEBUG
    spdlog::info(fmt::format("ref_block_width:{}, ref_block_height:{}",ref_block_width, ref_block_height));
#endif

    if(ref_block_width <= 0 || ref_block_height < 0){
        return return_msg(-2, fmt::format("ref_block_width({}) or ref_block_height({}) error(<0)",ref_block_width, ref_block_height));
    }

    if(ref_start_x + ref_block_width < 0 || ref_start_x + ref_block_width > ref_width - 1){
        return return_msg(-2, fmt::format("ref_start_x + ref_block_width:{} error, ref range:[0, {}]",ref_start_x + ref_block_width, ref_width - 1));
    }

    if(ref_start_y + ref_block_height < 0 || ref_start_y + ref_block_height > ref_height - 1){
        return return_msg(-2, fmt::format("ref_start_y + ref_block_height:{} error, ref range:[0, {}]",ref_start_y + ref_block_height, ref_height - 1));
    }

    void* arr = malloc(ref_block_width * ref_block_height * datatype_size);

    GDALRasterIOExtraArg ex_arg;
    INIT_RASTERIO_EXTRA_ARG(ex_arg);
    bool b_rasterio_resample = false;
    if(ref_height != tar_height || ref_height != tar_height){
        b_rasterio_resample = true;
        ex_arg.eResampleAlg = GDALRIOResampleAlg::GRIORA_Bilinear;
    }

    ref_rb->RasterIO(GF_Read, ref_start_x, ref_start_y, ref_block_width, ref_block_height, arr, tar_block_width, tar_block_height, ref_datatype, 0, 0, b_rasterio_resample ? &ex_arg : NULL);

    tar_rb->RasterIO(GF_Write, tar_start_x, tar_start_y, tar_block_width, tar_block_height, arr, tar_block_width, tar_block_height, tar_datatype, 0, 0);

    free(arr);

    GDALClose(tar_ds);
    GDALClose(ref_ds);
    
    return_msg(1, fmt::format("{} spend time:{}s",string(EXE_NAME),spend_time(start)));

    return return_msg(1, EXE_NAME " end.");
}