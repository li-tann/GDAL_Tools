#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"

using namespace std;
namespace fs = std::filesystem;

enum class method{unknown, point, txt, dem};

#ifndef PRINT_LOGGER
#define PRINT_LOGGER(LOGGER, TYPE, MASSAGE)  \
    LOGGER->TYPE(MASSAGE);                   \
    spdlog::TYPE(MASSAGE);                   
#endif


bool single(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);
bool multi(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);
bool _dem_(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int main(int argc, char* argv[])
{

    argparse::ArgumentParser program("create_delaunay", "1.0", argparse::default_arguments::help);
    program.add_description("input points, construct delaunay network and output");

    argparse::ArgumentParser sub_single("single");
    sub_single.add_description("print single point, and print result on terminal.");
    {
        sub_single.add_argument("longitude")
            .help("longitude")
            .scan<'g',float>();
        sub_single.add_argument("latitude")
            .help("latitude")
            .scan<'g',float>();

        sub_single.add_argument("height")
            .help("height")
            .scan<'g',float>();    

        sub_single.add_argument("ref_elevation_system")
            .help("the reference elevation system of input point, please input 'geodetic' or 'normal'. default is 'geodetic'")
            .default_value("geodetic")
            .choices("geodetic", "normal");
        
        sub_single.add_argument("-e","--egm_filepath")
            .help("input filepath  of 'Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE' EGM database.")
            .default_value("./data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE");
    }
    
    argparse::ArgumentParser sub_multi("multi");
    sub_multi.add_description("input a file(*.txt) for recording points info, and output a file(*.txt) for recording elevation correction info.");
    {
        sub_multi.add_argument("points_filepath")
            .help("record a point info on each line of the file, like 'lon, lat, height'.");

        sub_multi.add_argument("output_filepath")
            .help("record a point info on each line of the file, like 'lon, lat, height, egm_val, correction_height'.");

        sub_multi.add_argument("ref_elevation_system")
            .help("the reference elevation system of input point, please input 'geodetic' or 'normal'. default is 'geodetic'")
            .default_value("geodetic")
            .choices("geodetic", "normal");
        
        sub_multi.add_argument("-e","--egm_filepath")
            .help("input filepath  of 'Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE' EGM database.")
            .default_value("./data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE");
    }


    argparse::ArgumentParser sub_dem("dem");
    sub_dem.add_description("input a dem file, and output a elevation corrected dem (*.tif).");
    {
        sub_dem.add_argument("input_dem")
            .help("dem with float datatype.");

        sub_dem.add_argument("output_dem")
            .help("corrected dem with float datatype and tif driver, corrected_dem = dem +/- egm_val ('+':ref_system is normal, '-':ref_system is geodetic)");

        sub_dem.add_argument("ref_elevation_system")
            .help("the reference elevation system of input point, please input 'geodetic' or 'normal'. default is 'geodetic'")
            .default_value("geodetic")
            .choices("geodetic", "normal");

        sub_dem.add_argument("-e","--egm_filepath")
            .help("input filepath  of 'Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE' EGM database.")
            .default_value("./data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE");

        sub_dem.add_argument("-E","--egm_val")
            .help("write dem with value of egm2008, default is false, print '-E' or '--egm_val' means true")
            .default_value(false)
            .implicit_value(true);
    }


    std::map<argparse::ArgumentParser* , 
            std::function<int(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger>)>> 
    parser_map_func = {
        {&sub_single,   single},
        {&sub_multi,    multi},
        {&sub_dem,      _dem_},
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
    fs::path exe_root = fs::absolute(argv[0]).parent_path();
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
}

inline float get_corrected_height(float src_height, float abnormal_height, reference_elevation_system sys){
    float dst = src_height;
    switch (sys)
    {
    case normal:
        dst = src_height + abnormal_height;
        break;
    case geodetic:
        dst = src_height - abnormal_height;
        break;
    default:
        break;
    }
    return dst;
}

/*
    sub_single.add_argument("longitude")
        .help("longitude")
        .scan<'g',double>();
    sub_single.add_argument("latitude")
        .help("latitude")
        .scan<'g',double>();

    sub_single.add_argument("height")
        .help("height")
        .scan<'g',double>();    

    sub_single.add_argument("egm_filepath")
            .help("input filepath  of 'Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE' EGM database.");

    sub_single.add_argument("ref_elevation_system")
        .help("the reference elevation of input point, please input 'geodetic' or 'normal'. default is 'geodetic'")
        .default_value("geodetic");
*/
bool single(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    float lon = args->get<float>("longitude");
    float lat = args->get<float>("latitude");
    float hei = args->get<float>("height");
    string egm_filepath = args->get<string>("egm_filepath");

    string ref_system = args->get<string>("ref_elevation_system");
    if(ref_system != "normal" && ref_system != "geodetic"){
        PRINT_LOGGER(logger, error, "ref_system isn't 'normal' or 'geodetic'.");
        return false;
    }

    reference_elevation_system sys = geodetic;
    if(ref_system == "normal") 
        sys = normal;
    
    egm2008 egm;
    funcrst rst = egm.init(egm_filepath.c_str());
    if(!rst){
        PRINT_LOGGER(logger, error, fmt::format("egm.init failed, by '{}'.",rst.explain));
        return false;
    }

    cout<<"egm.height: "<<egm.height<<endl;
    cout<<"egm.width:  "<<egm.width<<endl;
    cout<<"egm.spacing:"<<egm.spacing<<endl;

    float height_anomaly = egm.calcluate_height_anomaly_single_point(lon, lat);

    PRINT_LOGGER(logger, info, fmt::format("longitude:        {}", lon));
    PRINT_LOGGER(logger, info, fmt::format("latitude:         {}", lat));
    PRINT_LOGGER(logger, info, fmt::format("height:           {}", hei));
    PRINT_LOGGER(logger, info, fmt::format("abnormal:         {}", height_anomaly));
    PRINT_LOGGER(logger, info, fmt::format("height_corrected: {}", get_corrected_height(hei, height_anomaly, sys)));

    PRINT_LOGGER(logger, info, "read_egm2008 single success.");
    return true;
}

/*
    sub_single.add_argument("points_filepath")
        .help("record a point info on each line of the file, like 'lon, lat, height'.");

    sub_single.add_argument("output_filepath")
        .help("record a point info on each line of the file, like 'lon, lat, height, egm_val, correction_height'.");

    sub_single.add_argument("egm_filepath")
        .help("input filepath  of 'Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE' EGM database.");

    sub_single.add_argument("ref_elevation_system")
        .help("the reference elevation system of input point, please input 'geodetic' or 'normal'. default is 'geodetic'")
        .default_value("geodetic");
*/

bool multi(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string input_filepath = args->get<string>("points_filepath");
    string output_filepath = args->get<string>("output_filepath");
    string egm_filepath = args->get<string>("egm_filepath");

    string ref_system = args->get<string>("ref_elevation_system");
    if(ref_system != "normal" && ref_system != "geodetic"){
        PRINT_LOGGER(logger, error, "ref_system isn't 'normal' or 'geodetic'.");
        return false;
    }

    reference_elevation_system sys = geodetic;
    if(ref_system == "normal") 
        sys = normal;


    egm2008 egm;
    funcrst rst = egm.init(egm_filepath.c_str());
    if(!rst){
        PRINT_LOGGER(logger, error, fmt::format("egm.init failed, by '{}'.",rst.explain));
        return false;
    }

    cout<<"egm.height: "<<egm.height<<endl;
    cout<<"egm.width:  "<<egm.width<<endl;
    cout<<"egm.spacing:"<<egm.spacing<<endl;


    rst = egm.write_height_anomaly_txt(input_filepath.c_str(), output_filepath.c_str(), sys);
    if(!rst){
        PRINT_LOGGER(logger, error, fmt::format("egm.write_height_anomaly_txt failed, by '{}'.",rst.explain));
        return false;
    }

    PRINT_LOGGER(logger, info, "read_egm2008 multi success.");
    return true;
}

/*
    sub_dem.add_argument("input_dem")
        .help("dem with float datatype.");

    sub_dem.add_argument("output_dem")
        .help("corrected dem with float datatype and tif driver, corrected_dem = dem +/- egm_val ('+':ref_system is normal, '-':ref_system is geodetic)");

    sub_single.add_argument("egm_filepath")
        .help("input filepath  of 'Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE' EGM database.");

    sub_dem.add_argument("ref_elevation_system")
        .help("the reference elevation system of input point, please input 'geodetic' or 'normal'. default is 'geodetic'")
        .default_value("geodetic");

    sub_dem.add_argument("-e","--egm_val")
        .help("write dem with value of egm2008.");
*/

bool _dem_(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string input_dem = args->get<string>("input_dem");
    string output_dem = args->get<string>("output_dem");
    string egm_filepath = args->get<string>("egm_filepath");

    string ref_system = args->get<string>("ref_elevation_system");
    if(ref_system != "normal" && ref_system != "geodetic"){
        PRINT_LOGGER(logger, error, "ref_system isn't 'normal' or 'geodetic'.");
        return false;
    }

    reference_elevation_system sys = geodetic;
    if(ref_system == "normal") 
        sys = normal;


    egm2008 egm;
    funcrst rst = egm.init(egm_filepath.c_str());
    if(!rst){
        PRINT_LOGGER(logger, error, fmt::format("egm.init failed, by '{}'.",rst.explain));
        return false;
    }

    cout<<"egm.height: "<<egm.height<<endl;
    cout<<"egm.width:  "<<egm.width<<endl;
    cout<<"egm.spacing:"<<egm.spacing<<endl;

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_in  = (GDALDataset*)GDALOpen(input_dem.c_str(), GA_ReadOnly);
    if(!ds_in){
        PRINT_LOGGER(logger, error, "ds_in is nullptr");
        return false;
    }
    GDALRasterBand* rb_in = ds_in->GetRasterBand(1);
    int dem_width = ds_in->GetRasterXSize();
    int dem_height = ds_in->GetRasterYSize();
    GDALDataType datatype_in = rb_in->GetRasterDataType();
    if(datatype_in != GDT_Float32){
        PRINT_LOGGER(logger, error, "datatype_in is not GDT_Float32");
        GDALClose(ds_in);
        return false;
    }
    double dem_gt[6];
    ds_in->GetGeoTransform(dem_gt);

    float* src_arr = new float[dem_height * dem_width];
    rb_in->RasterIO(GF_Read,0,0,dem_width, dem_height, src_arr,dem_width, dem_height,GDT_Float32,0,0);
    GDALClose(ds_in);

    GDALDriver* tif_driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = tif_driver->Create(output_dem.c_str(), dem_width, dem_height,1,GDT_Float32,nullptr);
    if(!ds_out){
        GDALClose(ds_in);
        PRINT_LOGGER(logger, error, "ds_out is nullptr");
        return false;
    }
    float* interped_arr = new float[dem_height * dem_width];
    rst = egm.write_height_anomaly_image(dem_height, dem_width, dem_gt, interped_arr);
    if(!rst){
        GDALClose(ds_in);
        GDALClose(ds_out);
        PRINT_LOGGER(logger, error, fmt::format("egm.write_height_anomaly_image failed, by '{}'.", rst.explain));
        return false;
    }

    /// 按需打印
    if(args->is_used("--egm_val")){
        fs::path path_output(output_dem);
        string egm_dem_filepath = path_output.replace_extension("egm.tif").string();
        cout<<"egm_dem_filepath:"<<egm_dem_filepath<<endl;
        GDALDataset* ds_egm = tif_driver->Create(egm_dem_filepath.c_str(), dem_width, dem_height,1,GDT_Float32,nullptr);
        if(!ds_egm){
            GDALClose(ds_in);
            PRINT_LOGGER(logger, warn, "ds_egm is nullptr");
        }
        else{
            GDALRasterBand* rb_egm = ds_egm->GetRasterBand(1);
            rb_egm->RasterIO(GF_Write,0,0,dem_width, dem_height, interped_arr,dem_width, dem_height,GDT_Float32,0,0);

            ds_egm->SetGeoTransform(dem_gt);
            ds_egm->SetProjection(ds_in->GetProjectionRef());

            GDALClose(ds_egm);
        }
    }

    for(int i=0; i<dem_height * dem_width; i++){
        interped_arr[i] = get_corrected_height(src_arr[i], interped_arr[i],sys);
    }
    // cout<<"interped_arr.size():"<<dynamic_array_size(interped_arr)<<endl;

    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);
    rb_out->RasterIO(GF_Write,0,0,dem_width, dem_height, interped_arr,dem_width, dem_height,GDT_Float32,0,0);

    ds_out->SetGeoTransform(dem_gt);
    ds_out->SetProjection(ds_in->GetProjectionRef());
    
    delete[] interped_arr;
    GDALClose(ds_out);
    
    PRINT_LOGGER(logger, info, "read_egm2008 dem success.");
    return true;
}