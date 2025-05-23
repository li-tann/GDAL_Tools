/**
 * @file main_raster.cpp
 * @author li-tann (li-tann@github.com)
 * @brief 
 * @version 0.1
 * @date 2024-05-01
 * 
 * @copyright Copyright (c) 2024
 * 
 */


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include <gdal_priv.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <argparse/argparse.hpp>

#include "raster_include.h"
#include "datatype.h"

namespace fs = std::filesystem;
using std::cout, std::cin, std::endl, std::string, std::vector, std::map;

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("gdal_tool_raster","1.0");
    program.add_description("gdal_tools about raster data, ...");
    
    argparse::ArgumentParser sub_value_translate("trans_val", "", argparse::default_arguments::help);
    sub_value_translate.add_description("translate the [source value] to [target value], and save as [output_imgpath].");
    {
        sub_value_translate.add_argument("input_imgpath")
            .help("the original image with the source value.");

        sub_value_translate.add_argument("output_imgpath")
            .help("the target image, if 'output_imgpath' and 'input_imgpath' are the same, directly operate on the input data.");

        sub_value_translate.add_argument("source_value")
            .help("source_value")
            .scan<'g',double>();
        
        sub_value_translate.add_argument("target_value", "")
            .help("target_value")
            .scan<'g',double>();
    }

    argparse::ArgumentParser sub_set_nodata("set_nodata", "", argparse::default_arguments::help);
    sub_set_nodata.add_description("set 'nodata value' metadata within image.");
    {
        sub_set_nodata.add_argument("img_filepath")
            .help("raster image filepath.");

        sub_set_nodata.add_argument("nodata_value")
            .help("nodata value")
            .scan<'g',double>();
    }

    argparse::ArgumentParser sub_statistics("stat_minmax", "", argparse::default_arguments::help);
    sub_statistics.add_description("statistics min, max, ave, std of the specified band of a image.");
    {
        sub_statistics.add_argument("img_filepath")
            .help("raster image filepath.");

        sub_statistics.add_argument("band")
            .help("band, default is 1.")
            .scan<'i',int>()
            .default_value("1");
    }

    argparse::ArgumentParser sub_histogram_stretch("stretch_hist", "", argparse::default_arguments::help);
    sub_histogram_stretch.add_description("remove the extreme values at both ends of the image based on proportion.");
    {
        sub_histogram_stretch.add_argument("input_imgpath")
            .help("input image filepath, doesn't support complex datatype");

        sub_histogram_stretch.add_argument("output_imgpath")
            .help("output image filepath (*.tif).");

        sub_histogram_stretch.add_argument("stretch_rate")
            .help("double, within (0,0.5], default is 0.02.")
            .scan<'g',double>()
            .default_value("0.02");    
    }

    argparse::ArgumentParser sub_histogram_statistics("stat_hist", "", argparse::default_arguments::help);
    sub_histogram_statistics.add_description("histogram statistics.");
    {
        sub_histogram_statistics.add_argument("input_imgpath")
            .help("input image filepath, doesn't support complex datatype");

        sub_histogram_statistics.add_argument("histogram_size")
            .help("int, histogram size, more than 1, default is 256.")
            .scan<'i',int>()
            .default_value("256");    
    }

    argparse::ArgumentParser sub_vrt_to_tif("vrt2tif", "", argparse::default_arguments::help);
    sub_vrt_to_tif.add_description("vrt image trans to tif");
    {
        sub_vrt_to_tif.add_argument("vrt")
            .help("input image filepath (*.vrt)");

        sub_vrt_to_tif.add_argument("tif")
            .help("output image filepath (*.tif)");    
    }

    argparse::ArgumentParser sub_tif_to_vrt("tif2vrt", "", argparse::default_arguments::help);
    sub_tif_to_vrt.add_description("tif image trans to vrt(binary), only trans the first band.");
    {
        sub_tif_to_vrt.add_argument("tif")
            .help("input image filepath");

        sub_tif_to_vrt.add_argument("binary")
            .help("output image filepath (binary, without extension)");
            
        sub_tif_to_vrt.add_argument("ByteOrder")
            .help("MSB(Most Significant Bit) or LSB(Least Significant Bit)")
            .default_value("MSB");
    }

    argparse::ArgumentParser sub_over_resample("resample", "", argparse::default_arguments::help);
    sub_over_resample.add_description("over resample by gdalwarp.");
    {
        sub_over_resample.add_argument("input_imgpath")
            .help("input image filepath");

        sub_over_resample.add_argument("output_imgpath")
            .help("output image filepath");
            
        sub_over_resample.add_argument("scale")
            .help("scale of over-resample.")
            .scan<'g',double>();

        sub_over_resample.add_argument("method")
            .help("over-resample method, use int to represent method : 0,nearst; 1,bilinear; 2,cubic; 3,cubicSpline; 4,lanczos(sinc).; 5,average.")
            .scan<'i',int>()
            .default_value("1")
            .choices("0","1","2","3","4","5");       
    }

    argparse::ArgumentParser sub_trans_geoinfo("trans_geo", "", argparse::default_arguments::help);
    sub_trans_geoinfo.add_description("copy source's geoinformation to target");
    {
        sub_trans_geoinfo.add_argument("source")
            .help("source image");

        sub_trans_geoinfo.add_argument("target")
            .help("target image");    
    }

    argparse::ArgumentParser sub_image_cut_pixel("image_cut", "", argparse::default_arguments::help);
    sub_image_cut_pixel.add_description("cut image by pixel, such as: input.tif output.tif 0 0 500 200");
    {
        sub_image_cut_pixel.add_argument("input_imgpath")
            .help("input image filepath");

        sub_image_cut_pixel.add_argument("output_imgpath")
            .help("output image filepath (*.tif)");   

        sub_image_cut_pixel.add_argument("pars")
            .help("4 pars with the order like: 'start_x start_y width height'(Use spaces as separators)")
            .scan<'i',int>()
            .nargs(4);        
    }

    argparse::ArgumentParser sub_image_overlay("image_overlay", "", argparse::default_arguments::help);
    sub_image_overlay.add_description("image_overlay, such as: upper.png lower.png overlay.png 0.5 0.9 normal");
    {
        sub_image_overlay.add_argument("upper_imgpath")
            .help("upper image filepath (4 band, 8bit *.png)");

        sub_image_overlay.add_argument("lower_imgpath")
            .help("lower image filepath (4 band, 8bit *.png)");

        sub_image_overlay.add_argument("overlay_imgpath")
            .help("overlaied image filepath (4 band, 8bit *.png)");   

        sub_image_overlay.add_argument("upper_opacity")
            .help("upper image's opacity 0~1")
            .scan<'g',double>();        

        sub_image_overlay.add_argument("lower_opacity")
            .help("lower image's opacity 0~1")
            .scan<'g',double>();

        sub_image_overlay.add_argument("method")
            .help("overlay method, such as: normal, premultiple, mask, additive, multiple, screen")
            .choices("normal","premultiple","mask","additive","multiple","screen");
    }

    argparse::ArgumentParser sub_image_set_colortable("image_color", "", argparse::default_arguments::help);
    sub_image_set_colortable.add_description("set colortable to byte image, not supporting 'CreateCopy' data");
    {
        sub_image_set_colortable.add_argument("img_path")
            .help("image filepath, with 1 band, 8bit, not supporting data in 'jpg', 'png' datatype");

        sub_image_set_colortable.add_argument("ct_path")
            .help("color table filepath (cm or cpt), set 'clear' if you want clear color table within image");
    }

    argparse::ArgumentParser sub_data_to_8bit("data_to_8bit", "", argparse::default_arguments::help);
    sub_data_to_8bit.add_description("convert data to 8bit");
    {
        sub_data_to_8bit.add_argument("img_path")
            .help("image filepath, supporting  data in 'byte', '(u)short', '(u)int', 'float', 'double' type");

        sub_data_to_8bit.add_argument("out_path")
            .help("out_path, which extension could be changed if it's diff with 'enxtension' par");

        sub_data_to_8bit.add_argument("extension")
            .help("extension, support jpg, png, bmp and tif");
    }

    argparse::ArgumentParser sub_grid_interp("grid_interp", "", argparse::default_arguments::help);
    sub_grid_interp.add_description("create a grid raster by some discrete points");
    {
        sub_grid_interp.add_argument("points_path")
            .help("points filepath, print x,y,z in per line");

        sub_grid_interp.add_argument("spacing")
            .help("spacing(resolution) of output raster image.")
            .scan<'g',double>();

        sub_grid_interp.add_argument("raster_path")
            .help("output raster filepath, which is a tiff image with 1 band, in float datatype.");
    }

    argparse::ArgumentParser sub_band_extract("band_extract", "", argparse::default_arguments::help);
    sub_band_extract.add_description("extract a single band and create a tif format data.");
    {
        sub_band_extract.add_argument("src")
            .help("src image");

        sub_band_extract.add_argument("bands")
            .help("bands number list, like: 1 2 3... ")
            .scan<'i',int>()
            .nargs(argparse::nargs_pattern::at_least_one);
    }

    argparse::ArgumentParser sub_vip_points("vip_points", "", argparse::default_arguments::help);
    sub_vip_points.add_description("extract important points in the first band of raster image (DEM), with datatype like short or float. ");
    {
        sub_vip_points.add_argument("input")
            .help("raster image (dem), with short or float datatype.");

        // sub_points_extract.add_argument("output_txt")
        //     .help("txt output filepath, like: pos0.y, pos0.x\\n pos1.y, pos1.x\\n... ");

        sub_vip_points.add_argument("output_mask_tif")
            .help("mask.tif with byte datatype, has the same coord with input tif.");
        
        sub_vip_points.add_argument("val_thres")
            .help("value threshold, compare with the abs diff between the cur_point's value and mean of surrounding points's value.")
            .scan<'g',double>();

        sub_vip_points.add_argument("output_type")
            .help("the unit of output points, like pixel or degree (same with geotransform's unit) (input 'pixel' or 'geo').")
            .choices("pixel","geo");

    }


    argparse::ArgumentParser sub_quadtree("quadtree", "", argparse::default_arguments::help);
    sub_quadtree.add_description("create quadtree base on a raster mask with byte datatype.");
    {
        sub_quadtree.add_argument("input")
            .help("raster image (dem), with short or float datatype.");

        sub_quadtree.add_argument("depth")
            .help("quadtree's max depth.")
            .scan<'i',int>();

        sub_quadtree.add_argument("output_jsonpath")
            .help("output jsonpath. ");

        sub_quadtree.add_argument("thres")
            .help("2 pars , the 1st par is method, like 'percent' or 'count', the 2nd par is percent(double, 0~1) or count(int, >0)")
            .nargs(2);        

    }

    argparse::ArgumentParser sub_jpg2png("jpg2png", "", argparse::default_arguments::help);
    sub_jpg2png.add_description("tranlate jpg to png.");
    {
        sub_jpg2png.add_argument("jpg")
            .help("jpg filepath with byte format.");

        sub_jpg2png.add_argument("png")
            .help("png filepath with byte format.");

        sub_jpg2png.add_argument("-a","--alpha")
            .help("the alpha band, the second parameter is the nodata-value map with RGB. ")
            .nargs(1);
       
    }

    argparse::ArgumentParser sub_triangle("triangle", "", argparse::default_arguments::help);
    sub_triangle.add_description("input mask image to generate triangle, and output with specify format.");
    {
        sub_triangle.add_argument("mask")
            .help("GDT_byte tif, with valid points marked as 1 and others marked as 0.");
        
        sub_triangle.add_argument("-f","--format")
            .help("output format, 'txt', 'tif', or 'bin'.")
            .default_value("txt")
            .choices("txt", "tif", "bin")
            .nargs(1);

        sub_triangle.add_argument("-p","--pointlist")
            .help("point list file with spcify format (*.txt, *.tif, *.pnts)")
            .nargs(1);

        sub_triangle.add_argument("-e","--edgelist")
            .help("edge list file with spcify format (*.txt, *.tif, *.edges)")
            .nargs(1);

        sub_triangle.add_argument("-t","--trianglelist")
            .help("-triangle list file with spcify format (*.txt, *.tif, *.tris)")
            .nargs(1);
    }
    
    argparse::ArgumentParser sub_geo2pix("cal_geo2pix", "", argparse::default_arguments::help);
    sub_geo2pix.add_description("input geo_img, geo-coord points (one or more), and print corresponding pix-coord points.");
    {
        sub_geo2pix.add_argument("geo_img")
            .help("image with valid geotransform");
        
        sub_geo2pix.add_argument("-p","--pointlist")
            .help("geo point list, like 'lon1,lat1 lon2,lat2'")
            .nargs(argparse::nargs_pattern::at_least_one);

    }

    argparse::ArgumentParser sub_pix2geo("cal_pix2geo", "", argparse::default_arguments::help);
    sub_pix2geo.add_description("input geo_img, pix-coord points (one or more), and print corresponding geo-coord points.");
    {
        sub_pix2geo.add_argument("geo_img")
            .help("image with valid geotransform");
        
        sub_pix2geo.add_argument("-p","--pointlist")
            .help("pixel point list, like 'y1,x1 y2,x2'")
            .nargs(argparse::nargs_pattern::at_least_one);

    }

    std::map<argparse::ArgumentParser* , 
            std::function<int(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger>)>> 
    parser_map_func = {
        {&sub_value_translate,      value_translate},
        {&sub_set_nodata,           set_nodata_value},
        {&sub_statistics,           statistics},
        {&sub_histogram_stretch,    histogram_stretch},
        {&sub_histogram_statistics, histogram_statistics},
        {&sub_vrt_to_tif,           vrt_to_tif},
        {&sub_tif_to_vrt,           tif_to_vrt},
        {&sub_over_resample,        over_resample},
        {&sub_trans_geoinfo,        trans_geoinformation},
        {&sub_image_cut_pixel,      image_cut_by_pixel},
        {&sub_image_overlay,        image_overlay},
        {&sub_image_set_colortable, image_set_colortable},
        {&sub_data_to_8bit,         data_convert_to_byte},
        {&sub_grid_interp,          grid_interp},
        {&sub_band_extract,         band_extract},
        {&sub_vip_points,           import_points_extract},
        {&sub_triangle,             triangle_network},
        {&sub_quadtree,             create_quadtree},
        {&sub_jpg2png,              jpg_to_png},
        {&sub_geo2pix,              cal_geo2pix},
        {&sub_pix2geo,              cal_pix2geo},
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
    fs::path log_path = exe_root / "gdal_tool_raster.log";
    auto my_logger = spdlog::basic_logger_mt("gdal_tool_raster", log_path.string());

    std::string config;
    for(int i=0; i<argc; i++){
        config += std::string(argv[i]) + " ";
    }
    PRINT_LOGGER(my_logger, info, "gdal_tool_raster start");
    PRINT_LOGGER(my_logger, info, fmt::format("config:\n[{}]",config));
    auto time_start = std::chrono::system_clock::now();

    for(auto& iter : parser_map_func){
        if(program.is_subcommand_used(*(iter.first))){
            return iter.second(iter.first, my_logger);
        } 
    }
    
    PRINT_LOGGER(my_logger, info, fmt::format("gdal_tool_raster end, spend time {}s",spend_time(time_start)));
    return 0;
}