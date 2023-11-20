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

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# 空值转换为指定数值
add_executable(_nan_TransTo nan_transto.cpp template_nan_convert_to.h)
target_link_libraries(_nan_TransTo PRIVATE GDAL::GDAL)
target_link_libraries(_nan_TransTo PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# setNoDataValue 去黑边
add_executable(_set_NoData_Value set_nodata_value.cpp)
target_link_libraries(_set_NoData_Value PRIVATE GDAL::GDAL)
target_link_libraries(_set_NoData_Value PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# float格式（不止）的二进制影像转换为tif并输出
add_executable(_binary_to_tif binary_img_to_tif.cpp template_binary_img_io.h)
target_link_libraries(_binary_to_tif PRIVATE GDAL::GDAL)
target_link_libraries(_binary_to_tif PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# tif转换为二进制格式
add_executable(_tif_to_binary tif_to_binary.cpp template_binary_img_io.h)
target_link_libraries(_tif_to_binary PRIVATE GDAL::GDAL)
target_link_libraries(_tif_to_binary PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 统计影像信息
add_executable(_statistics statistics.cpp)
target_link_libraries(_statistics PRIVATE GDAL::GDAL)
target_link_libraries(_statistics PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#百分比拉伸
add_executable(_histogram_stretch histogram_stretch.cpp template_stretch.h)
target_link_libraries(_histogram_stretch PRIVATE GDAL::GDAL)
target_link_libraries(_histogram_stretch PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#创建delaunay
add_executable(_create_delaunay CreateDelaunay.cpp)
target_link_libraries(_create_delaunay PRIVATE GDAL::GDAL)
target_link_libraries(_create_delaunay PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#判断点是否在shp文件中
add_executable(_determine_topological_relationship DetermineTopologicalRelationship.cpp)
target_link_libraries(_determine_topological_relationship PRIVATE GDAL::GDAL)
target_link_libraries(_determine_topological_relationship PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# 打印sar影像
add_executable(_print_sar_image print_sar_image.cpp template_sarImage_process.h datatype.cpp)
target_include_directories(	_print_sar_image 
							INTERFACE 
							${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(_print_sar_image PRIVATE GDAL::GDAL)
target_link_libraries(_print_sar_image PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(_print_sar_image PRIVATE fmt::fmt)

# 融合sar影像
add_executable(_merge_sar_image merge_sar_image.cpp template_sarImage_process.h)
target_link_libraries(_merge_sar_image PRIVATE GDAL::GDAL)
target_link_libraries(_merge_sar_image PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

# duqu EGM2008文件, 并写出
add_executable(_read_egm2008 read_egm2008.cpp datatype.cpp)
target_include_directories(	_read_egm2008 
							INTERFACE 
							${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(_read_egm2008 PRIVATE GDAL::GDAL)
target_link_libraries(_read_egm2008 PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(_read_egm2008 PRIVATE fmt::fmt)

add_executable(_vrt_io vrt_test.cpp)
target_link_libraries(_vrt_io PRIVATE GDAL::GDAL)
target_link_libraries(_vrt_io PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

add_executable(_vrt_trans vrt_trans.cpp)
target_link_libraries(_vrt_trans PRIVATE GDAL::GDAL)
target_link_libraries(_vrt_trans PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

add_executable(_fcpx_filter fcpx_filter.cpp datatype.h)
target_link_libraries(_fcpx_filter PRIVATE GDAL::GDAL)
target_link_libraries(_fcpx_filter PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

add_executable(_over_resample over_resample.cpp datatype.h)
target_link_libraries(_over_resample PRIVATE GDAL::GDAL)
target_link_libraries(_over_resample PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

add_executable(_transmit_geoinformation transmit_geoinformation.cpp)
target_link_libraries(_transmit_geoinformation PRIVATE GDAL::GDAL)
target_link_libraries(_transmit_geoinformation PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

add_executable(_unified_geoimage_merging unified_GeoImage_merging.cpp)
target_link_libraries(_unified_geoimage_merging PRIVATE GDAL::GDAL)
target_link_libraries(_unified_geoimage_merging PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

install(TARGETS _transmit_geoinformation
		RUNTIME DESTINATION exes_bin)