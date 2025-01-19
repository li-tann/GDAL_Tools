#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include <gdal_priv.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <argparse/argparse.hpp>
#include "datatype.h"

namespace fs = std::filesystem;
using std::cout, std::cin, std::endl, std::string, std::vector, std::map;

#ifndef PRINT_LOGGER
#define PRINT_LOGGER(LOGGER, TYPE, MASSAGE)  \
    LOGGER->TYPE(MASSAGE);                   \
    spdlog::TYPE(MASSAGE);                   
#endif


int value_translate(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int set_nodata_value(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int statistics(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int histogram_stretch(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int histogram_statistics(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int vrt_to_tif(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int tif_to_vrt(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int over_resample(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int trans_geoinformation(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int image_cut_by_pixel(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int image_overlay(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int image_set_colortable(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int data_convert_to_byte(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger);

int grid_interp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int band_extract(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int import_points_extract(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int create_quadtree(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int jpg_to_png(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int triangle_network(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);