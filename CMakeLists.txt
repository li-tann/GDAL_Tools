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

# float格式（不止）的二进制影像转换为tif并输出
add_executable(binary_to_tif binary_img_to_tif.cpp template_binary_img_io.h)
target_link_libraries(binary_to_tif PRIVATE GDAL::GDAL)
target_link_libraries(binary_to_tif PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

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

#创建delaunay
add_executable(create_delaunay CreateDelaunay.cpp)
target_link_libraries(create_delaunay PRIVATE GDAL::GDAL)
target_link_libraries(create_delaunay PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

#判断点是否在shp文件中
add_executable(determine_topological_relationship DetermineTopologicalRelationship.cpp)
target_link_libraries(determine_topological_relationship PRIVATE GDAL::GDAL)
target_link_libraries(determine_topological_relationship PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(determine_topological_relationship PRIVATE fmt::fmt)

# 打印sar影像
add_executable(print_sar_image print_sar_image.cpp template_sarImage_process.h datatype.cpp)
target_include_directories(print_sar_image 
							INTERFACE 
							${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(print_sar_image PRIVATE GDAL::GDAL)
target_link_libraries(print_sar_image PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(print_sar_image PRIVATE fmt::fmt)

# 融合sar影像
add_executable(merge_sar_image merge_sar_image.cpp template_sarImage_process.h)
target_link_libraries(merge_sar_image PRIVATE GDAL::GDAL)
target_link_libraries(merge_sar_image PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

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

# 复数二维数组滤波
add_executable(fcpx_filter fcpx_filter.cpp datatype.h)
target_link_libraries(fcpx_filter PRIVATE GDAL::GDAL)
target_link_libraries(fcpx_filter PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
if(OpenMP_FOUND)
    target_link_libraries(fcpx_filter PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

# 过采样
add_executable(over_resample over_resample.cpp datatype.h)
target_link_libraries(over_resample PRIVATE GDAL::GDAL)
target_link_libraries(over_resample PRIVATE spdlog::spdlog spdlog::spdlog_header_only)


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

# 从数据B中复制一块区域填充到A数据中(几何关系一致)
add_executable(geoimage_copy_ref_paste_to_target copy_Geoimage_paste.cpp datatype.h datatype.cpp)
target_link_libraries(geoimage_copy_ref_paste_to_target PRIVATE GDAL::GDAL)
target_link_libraries(geoimage_copy_ref_paste_to_target PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(geoimage_copy_ref_paste_to_target PRIVATE fmt::fmt)


# rolling guidance 滤波
add_executable(rolling_guidance_filter rolling_guidance.cpp datatype.h datatype.cpp)
target_link_libraries(rolling_guidance_filter PRIVATE GDAL::GDAL)
target_link_libraries(rolling_guidance_filter PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(rolling_guidance_filter PRIVATE fmt::fmt)
if(OpenMP_FOUND)
    target_link_libraries(rolling_guidance_filter PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

#复数影像格式转换
add_executable(complex_transto complex_transto.cpp datatype.h datatype.cpp)
target_link_libraries(complex_transto PRIVATE GDAL::GDAL)
target_link_libraries(complex_transto PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(complex_transto PRIVATE fmt::fmt)

#基于像素的影像裁剪
add_executable(image_cut_pixel image_cut_pixel.cpp datatype.h datatype.cpp)
target_link_libraries(image_cut_pixel PRIVATE GDAL::GDAL)
target_link_libraries(image_cut_pixel PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(image_cut_pixel PRIVATE fmt::fmt)

#滤波评价
add_executable(filter_evaluation filter_evaluation.cpp datatype.h datatype.cpp)
target_link_libraries(filter_evaluation PRIVATE GDAL::GDAL)
target_link_libraries(filter_evaluation PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(filter_evaluation PRIVATE fmt::fmt)

#获取图像在某条直线上的值
add_executable(get_image_value_in_line get_image_value_in_line.cpp datatype.h datatype.cpp)
target_link_libraries(get_image_value_in_line PRIVATE GDAL::GDAL)
target_link_libraries(get_image_value_in_line PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(get_image_value_in_line PRIVATE fmt::fmt)

#goldstein滤波
add_executable(goldstein goldstein.cpp datatype.h datatype.cpp)
target_link_libraries(goldstein PRIVATE GDAL::GDAL)
target_link_libraries(goldstein PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(goldstein PRIVATE fmt::fmt)
target_link_libraries(goldstein PRIVATE FFTW3::fftw3f)
if(OpenMP_FOUND)
    target_link_libraries(goldstein PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

#pseudo-coherence 伪相干性计算
add_executable(pseudo_correlation pseudo_correlation.cpp datatype.h datatype.cpp)
target_link_libraries(pseudo_correlation PRIVATE GDAL::GDAL)
target_link_libraries(pseudo_correlation PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(pseudo_correlation PRIVATE fmt::fmt)
if(OpenMP_FOUND)
    target_link_libraries(pseudo_correlation PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

#goldstein-baran滤波
add_executable(baran goldstein_baran.cpp datatype.h datatype.cpp)
target_link_libraries(baran PRIVATE GDAL::GDAL)
target_link_libraries(baran PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(baran PRIVATE fmt::fmt)
target_link_libraries(baran PRIVATE FFTW3::fftw3f)
if(OpenMP_FOUND)
    target_link_libraries(baran PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

#goldstein-zhao滤波
add_executable(zhao goldstein_zhao.cpp datatype.h datatype.cpp)
target_link_libraries(zhao PRIVATE GDAL::GDAL)
target_link_libraries(zhao PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(zhao PRIVATE fmt::fmt)
target_link_libraries(zhao PRIVATE FFTW3::fftw3f)
if(OpenMP_FOUND)
    target_link_libraries(zhao PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

#interf_spectrum_filter滤波
add_executable(interf_spectrum_filter interf_spectrum_filter.cpp datatype.h datatype.cpp)
target_link_libraries(interf_spectrum_filter PRIVATE GDAL::GDAL)
target_link_libraries(interf_spectrum_filter PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(interf_spectrum_filter PRIVATE fmt::fmt)
target_link_libraries(interf_spectrum_filter PRIVATE FFTW3::fftw3f)
if(OpenMP_FOUND)
    target_link_libraries(interf_spectrum_filter PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()

#interf 很简单的干涉
add_executable(interf interf.cpp datatype.h datatype.cpp)
target_link_libraries(interf PRIVATE GDAL::GDAL)
target_link_libraries(interf PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(interf PRIVATE fmt::fmt)
if(OpenMP_FOUND)
    target_link_libraries(interf PRIVATE OpenMP::OpenMP_CXX)
else()
    message(FATAL_ERROR "openmp not found!")
endif()


# add "cmake.configureSettings": {"CMAKE_BUILD_TYPE":"${buildType}"} in setting.json, when ${CMAKE_BUILD_TYPE} is ""
message(STATUS "cmake build type: " ${CMAKE_BUILD_TYPE})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE})
file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/proj.db ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/proj.db ONLY_IF_DIFFERENT)

install(TARGETS transmit_geoinformation
		RUNTIME DESTINATION exes_bin)