#ifndef INSAR_INCLUDE_H
#define INSAR_INCLUDE_H


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

#include <fftw3.h>
#include <omp.h>

namespace fs = std::filesystem;
using std::cout, std::cin, std::endl, std::string, std::vector, std::map;

#ifndef PRINT_LOGGER
#define PRINT_LOGGER(LOGGER, TYPE, MASSAGE)  \
    LOGGER->TYPE(MASSAGE);                   \
    spdlog::TYPE(MASSAGE);                   
#endif


int filter_goldstein(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int filter_goldstein_zhao(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

int filter_goldstein_baran(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger);

#endif