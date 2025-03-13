#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>

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

int point_with_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int polygen_with_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int polygen_overlap_rate(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int create_polygon_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int create_2dpoint_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int create_3dpoint_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int create_linestring_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int points_shapefile_dilution(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);