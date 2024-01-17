cmake_minimum_required(VERSION 3.0.0)
project(execs VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(CTest)
enable_testing()

find_package(GDAL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

#debug config 测试
add_executable(debug_config_test debug_config_test.cpp)

# 空值转换为指定数值
add_executable(nan_TransTo nan_transto.cpp template_nan_convert_to.h)
target_link_libraries(nan_TransTo PRIVATE GDAL::GDAL)
target_link_libraries(nan_TransTo PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# setNoDataValue 去黑边
add_executable(set_NoData_Value set_nodata_value.cpp)
target_link_libraries(set_NoData_Value PRIVATE GDAL::GDAL)
target_link_libraries(set_NoData_Value PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# tif转换为二进制格式
add_executable(tif_to_binary tif_to_binary.cpp template_binary_img_io.h)
target_link_libraries(tif_to_binary PRIVATE GDAL::GDAL)
target_link_libraries(tif_to_binary PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 统计影像信息
add_executable(statistics statistics.cpp)
target_link_libraries(statistics PRIVATE GDAL::GDAL)
target_link_libraries(statistics PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#百分比拉伸
add_executable(histogram_stretch histogram_stretch.cpp template_stretch.h)
target_link_libraries(histogram_stretch PRIVATE GDAL::GDAL)
target_link_libraries(histogram_stretch PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 直方图统计
add_executable(histogram_statistics histogram_statistics.cpp template_stretch.h)
target_link_libraries(histogram_statistics PRIVATE GDAL::GDAL)
target_link_libraries(histogram_statistics PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#创建delaunay
add_executable(create_delaunay CreateDelaunay.cpp)
target_link_libraries(create_delaunay PRIVATE GDAL::GDAL)
target_link_libraries(create_delaunay PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#判断点是否在shp文件中
add_executable(determine_topological_relationship DetermineTopologicalRelationship.cpp)
target_link_libraries(determine_topological_relationship PRIVATE GDAL::GDAL)
target_link_libraries(determine_topological_relationship PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(determine_topological_relationship PRIVATE fmt::fmt)

# duqu EGM2008文件, 并写出
add_executable(read_egm2008 read_egm2008.cpp datatype.cpp)
target_include_directories( read_egm2008 
							INTERFACE 
							${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(read_egm2008 PRIVATE GDAL::GDAL)
target_link_libraries(read_egm2008 PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(read_egm2008 PRIVATE fmt::fmt)

# vrt数据的读写(测试)
add_executable(vrt_io vrt_test.cpp)
target_link_libraries(vrt_io PRIVATE GDAL::GDAL)
target_link_libraries(vrt_io PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# vrt与tif格式相互转换
add_executable(vrt_trans vrt_trans.cpp)
target_link_libraries(vrt_trans PRIVATE GDAL::GDAL)
target_link_libraries(vrt_trans PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 过采样
add_executable(over_resample over_resample.cpp datatype.h)
target_link_libraries(over_resample PRIVATE GDAL::GDAL)
target_link_libraries(over_resample PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 传递地理坐标
add_executable(transmit_geoinformation transmit_geoinformation.cpp)
target_link_libraries(transmit_geoinformation PRIVATE GDAL::GDAL)
target_link_libraries(transmit_geoinformation PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 同一坐标系统的分块数据拼接
add_executable(unified_geoimage_merging unified_GeoImage_merging.cpp datatype.h datatype.cpp)
target_link_libraries(unified_geoimage_merging PRIVATE GDAL::GDAL)
target_link_libraries(unified_geoimage_merging PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
if(OpenMP_FOUND)
    target_link_libraries(unified_geoimage_merging PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

# 创建shp文件
add_executable(create_shapfile create_shapfile.cpp datatype.h datatype.cpp)
target_link_libraries(create_shapfile PRIVATE GDAL::GDAL)
target_link_libraries(create_shapfile PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#基于像素的影像裁剪
add_executable(image_cut_pixel image_cut_pixel.cpp datatype.h datatype.cpp)
target_link_libraries(image_cut_pixel PRIVATE GDAL::GDAL)
target_link_libraries(image_cut_pixel PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(image_cut_pixel PRIVATE fmt::fmt)

#获取图像在某条直线上的值
add_executable(get_image_value_in_line get_image_value_in_line.cpp datatype.h datatype.cpp)
target_link_libraries(get_image_value_in_line PRIVATE GDAL::GDAL)
target_link_libraries(get_image_value_in_line PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(get_image_value_in_line PRIVATE fmt::fmt)

#GDAL虚拟文件系统测试
add_executable(virtual_files_system_test virtual_files_system_test.cpp datatype.h datatype.cpp)
target_link_libraries(virtual_files_system_test PRIVATE GDAL::GDAL)
target_link_libraries(virtual_files_system_test PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(virtual_files_system_test PRIVATE fmt::fmt)


# add "cmake.configureSettings": {"CMAKE_BUILD_TYPE":"${buildType}"} in setting.json, when ${CMAKE_BUILD_TYPE} is ""
message(STATUS "cmake build type: " ${CMAKE_BUILD_TYPE})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE})
file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/proj.db ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/proj.db ONLY_IF_DIFFERENT)

install(TARGETS transmit_geoinformation
		RUNTIME DESTINATION exes_bin)