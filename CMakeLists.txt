cmake_minimum_required(VERSION 3.0.0)
project(execs VERSION 0.1.0)

include(CTest)
enable_testing()

find_package(GDAL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_executable(_NAN-Points_Fill_in_Val nan_point_fill_in.cpp)
target_link_libraries(_NAN-Points_Fill_in_Val PRIVATE GDAL::GDAL)
target_link_libraries(_NAN-Points_Fill_in_Val PRIVATE spdlog::spdlog spdlog::spdlog_header_only)