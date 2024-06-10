/**
 * @file main_vector.cpp
 * @author li-tann (li-tann@github.com)
 * @brief 
 * @version 0.1
 * @date 2024-05-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "vector_include.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("gdal_tool_vector","1.0");
    program.add_description("gdal_tools about vector data, ...");

    argparse::ArgumentParser sub_point_with_shp("point_with_shp");
    sub_point_with_shp.add_description("determine relationship between point and shapefile.");
    {
        sub_point_with_shp.add_argument("shapefile_path")
            .help("polygon shapefile.");
        
        sub_point_with_shp.add_argument("-p","--points")
            .help("point like: 'x,y' or 'lon,lat'.")
            .nargs(argparse::nargs_pattern::at_least_one);

        sub_point_with_shp.add_argument("-f","--file")
            .help("a file that records points, with each line representing a point like: 'x,y' or 'lon,lat'.");

        sub_point_with_shp.add_argument("-s","--save")
            .help("save result as a file, default is print at terminal.");    
    }

    argparse::ArgumentParser sub_create_polygon_shp("create_polygon_shp");
    sub_create_polygon_shp.add_description("create a polygon shapefile on WGS84 coordination,  base on points(file or cin).");
    {
        sub_create_polygon_shp.add_argument("output_shapefile")
            .help("output shapefile filepath");

        sub_create_polygon_shp.add_argument("-p", "--points")
            .help("point like: 'lon,lat'.")
            .remaining();

        sub_create_polygon_shp.add_argument("-f", "--file")
            .help("a file that records points, with each line representing a point like: 'lon,lat'. (--file will covered --points)");
    }

    argparse::ArgumentParser sub_create_2dpoint_shp("create_2dpoint_shp");
    sub_create_2dpoint_shp.add_description("create a 2d-point shapefile on WGS84 coordination, base on points(file or cin).");
    {
        sub_create_2dpoint_shp.add_argument("output_shapefile")
            .help("output shapefile filepath");

        sub_create_2dpoint_shp.add_argument("-p", "--points")
            .help("point like: 'lon,lat'.")
            .nargs(argparse::nargs_pattern::at_least_one);

        sub_create_2dpoint_shp.add_argument("-f", "--file")
            .help("a file that records points, with each line representing a point like: 'lon,lat'.");
    }

    argparse::ArgumentParser sub_create_3dpoint_shp("create_3dpoint_shp");
    sub_create_3dpoint_shp.add_description("create a 3d-point shapefile on WGS84 coordination, base on points(file or cin).");
    {
        sub_create_3dpoint_shp.add_argument("output_shapefile")
            .help("output shapefile filepath");

        sub_create_3dpoint_shp.add_argument("-p", "--points")
            .help("point like: 'lon,lat,val'.")
            .nargs(argparse::nargs_pattern::at_least_one);

        sub_create_3dpoint_shp.add_argument("-f", "--file")
            .help("a file that records points, with each line representing a point like: 'lon,lat,val'.");
    }

    std::map<argparse::ArgumentParser* , 
            std::function<int(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger>)>> 
    parser_map_func = {
        {&sub_point_with_shp,      point_with_shp},
        {&sub_create_polygon_shp,  create_polygon_shp},
        {&sub_create_2dpoint_shp,  create_2dpoint_shp},
        {&sub_create_3dpoint_shp,  create_3dpoint_shp},
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
    fs::path log_path = exe_root / "gdal_tool_vector.log";
    auto my_logger = spdlog::basic_logger_mt("gdal_tool_vector", log_path.string());

    std::string config;
    for(int i=0; i<argc; i++){
        config += std::string(argv[i]) + " ";
    }
    PRINT_LOGGER(my_logger, info, "gdal_tool_vector start");
    PRINT_LOGGER(my_logger, info, fmt::format("config:[{}]",config));
    auto time_start = std::chrono::system_clock::now();

    for(auto& iter : parser_map_func){
        if(program.is_subcommand_used(*(iter.first))){
            return iter.second(iter.first, my_logger);
        } 
    }
    PRINT_LOGGER(my_logger, info, fmt::format("gdal_tool_vector end, spend time {}s",spend_time(time_start)));
    return 1;
}