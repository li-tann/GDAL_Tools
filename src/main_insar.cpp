/**
 * @file main_insar.cpp
 * @author li-tann (li-tann@github.com)
 * @brief tools for sar/insar data
 * @version 0.1
 * @date 2024-06-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "insar_include.h"

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("gdal_tool_insar","1.0");
    program.add_description("gdal_tools about sar or insar data, ...");

    argparse::ArgumentParser sub_goldstein("goldstein");
    sub_goldstein.add_description("filter insar data like int(cfloat32) or int.phase(float), with method goldstein.");
    {
        sub_goldstein.add_argument("input_path")
            .help("input file path, support float and fcomplex.");
        
        sub_goldstein.add_argument("output_path")
            .help("filterd output file path, with same datatype with input_path.");

        sub_goldstein.add_argument("-a","--alpha")
            .help("the power of window, within range [0,1], , default is 0.5. The higher the power, the more pronounced the filtering effect.")
            .scan<'g',double>()
            .default_value("0.5");   
    }

    argparse::ArgumentParser sub_goldstein_f("goldstein_f");
    sub_goldstein_f.add_description("filter float data, with method goldstein.");
    {
        sub_goldstein_f.add_argument("input_path")
            .help("input file path, support float.");
        
        sub_goldstein_f.add_argument("output_path")
            .help("filterd output file path, with same datatype with input_path.");

        sub_goldstein_f.add_argument("-a","--alpha")
            .help("the power of window, within range [0,1], , default is 0.5. The higher the power, the more pronounced the filtering effect.")
            .scan<'g',double>()
            .default_value("0.5");   
    }

    argparse::ArgumentParser sub_goldstein_zhao("zhao");
    sub_goldstein_zhao.add_description("filter insar data like int(cfloat32), with method zhao-filter.");
    {
        sub_goldstein_zhao.add_argument("input_path")
            .help("input file path, support float and fcomplex.");
        
        sub_goldstein_zhao.add_argument("output_path")
            .help("filterd output file path, with same datatype with input_path.");

        sub_goldstein_zhao.add_argument("-a","--alpha_outpath")
            .help("optional output the alpha_output_path, with recorded the aplha used with goldstein in every window");   
    }

    argparse::ArgumentParser sub_goldstein_baran("baran");
    sub_goldstein_baran.add_description("filter insar data like int(cfloat32), with method baran-filter.");
    {
        sub_goldstein_baran.add_argument("input_path")
            .help("input int file path, support float and fcomplex.");

        sub_goldstein_baran.add_argument("cor_path")
            .help("input correlation file path, support float.");
        
        sub_goldstein_baran.add_argument("output_path")
            .help("filterd output file path, with same datatype with input_path.");

        sub_goldstein_baran.add_argument("-a","--alpha_outpath")
            .help("optional output the alpha_output_path, with recorded the aplha used with goldstein in every window");   
    }

    argparse::ArgumentParser sub_pseudo_correlation("pseudo");
    sub_pseudo_correlation.add_description("calculate the pseudo correlation data, from complex data.");
    {
        sub_pseudo_correlation.add_argument("input_path")
            .help("input int file path, with fcomplex datatype.");

        sub_pseudo_correlation.add_argument("output_path")
            .help("output pseudo correlation filepath, with float datatype.");
        
        sub_pseudo_correlation.add_argument("win_size")
            .help("the window size used to calculate pseudo correlation, odd integer, default is 5.")
            .scan<'i',int>()
            .default_value("5"); 
    }

    argparse::ArgumentParser sub_interf("interf");
    sub_interf.add_description("conjugate multiplication of data and filtering along azimuth and slant range direction.");
    {
        sub_interf.add_argument("master_data_path")
            .help("master fcomplex data, with fcomplex datatype.");

        sub_interf.add_argument("slave_data_path")
            .help("slave fcomplex data, with float datatype.");
        
        sub_interf.add_argument("output_interf_path")
            .help("output interf data (hase been conjugate multiplicated)."); 

        sub_interf.add_argument("-a","--azimuth_pars")
            .help("azimuth parameters in the following order: mas_az_bw mas_az_freq sla_az_bw sla_az_freq.");

        sub_interf.add_argument("-r","--range_pars")
            .help("slant-range parameters in the following order: mas_rg_bw mas_rg_freq sla_rg_bw sla_rg_freq.");

    /// TODO: 创建相关的函数
    }

    argparse::ArgumentParser sub_8bitdata("8bitdata");
    sub_8bitdata.add_description("create 8-bit data, base on slc or mli data.");
    {
        sub_8bitdata.add_argument("input_path")
            .help("valid datatype include short, float, scomplex, fcomplex.");

        sub_8bitdata.add_argument("output_path")
            .help(" valid extension include, .tif(1b+ct), .bmp(1b+ct), .jpg(4b), .png(4b)."); 

        sub_8bitdata.add_argument("scale")
            .help("dst_value = scale * (src_value - average)^ ex")
            .scan<'g',double>()
            .default_value("1");

        sub_8bitdata.add_argument("ex")
            .help("dst_value = scale * (src_value - average)^ ex")
            .scan<'g',double>()
            .default_value("0.35");

        sub_8bitdata.add_argument("-t","--type")
            .help("this parameter is useful, only when datatype of input_path is complex.")
            .choices("power","phase");

    /// TODO: 创建相关的函数
    }


    std::map<argparse::ArgumentParser* , 
            std::function<int(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger>)>> 
    parser_map_func = {
        {&sub_goldstein,            filter_goldstein},
        {&sub_goldstein_f,          filter_goldstein_float},
        {&sub_goldstein_zhao,       filter_goldstein_zhao},
        {&sub_goldstein_baran,      filter_goldstein_baran},
        {&sub_pseudo_correlation,   pseudo_correlation},
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
    fs::path log_path = exe_root / "gdal_tool_insar.log";
    auto my_logger = spdlog::basic_logger_mt("gdal_tool_insar", log_path.string());

    std::string config;
    for(int i=0; i<argc; i++){
        config += std::string(argv[i]) + " ";
    }
    PRINT_LOGGER(my_logger, info, "gdal_tool_insar start");
    PRINT_LOGGER(my_logger, info, fmt::format("config:\n[{}]",config));
    auto time_start = std::chrono::system_clock::now();

    for(auto& iter : parser_map_func){
        if(program.is_subcommand_used(*(iter.first))){
            return iter.second(iter.first, my_logger);
        } 
    }
    PRINT_LOGGER(my_logger, info, fmt::format("gdal_tool_insar end, spend time {}s",spend_time(time_start)));
    return 1;
}