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
find_package(argparse CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
set(nlohmann_json_IMPLICIT_CONVERSIONS OFF)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

set(EXE_LIST)

# main_vector & main_raster
add_executable(gdal_tool_raster
        src/main_raster.cpp
        src/datatype.h
        src/datatype.cpp
        src/raster_include.h     
        src/value_translate.cpp         # A转换为B
        src/template_nan_convert_to.h       # template A转换为B
        src/set_nodata_value.cpp        # 设置NoData值
        src/statistics.cpp              # 栅格信息统计
        src/histogram_stretch.cpp       # 栅格百分比拉伸
        src/histogram_statistics.cpp    # 直方图统计
        src/template_stretch.h              # template 栅格百分比拉伸 & 直方图统计
        src/vrt_trans.cpp               # vrt与tif格式相互转换
        src/binary_write.h                  # template vrt与tif格式相互转换
        src/over_resample.cpp           # 过采样
        src/transmit_geoinformation.cpp # 传递地理坐标
        src/image_cut_pixel.cpp         # 基于像素的影像裁剪
        src/image_overlay.cpp           # 影像叠加
        src/image_set_colortable.cpp    # 影像添加color table (only for 8bit data)
        src/data_convert_to_byte.cpp    # 影像转8bit图
        src/grid_interp.cpp             # 基于离散点生成栅格图
        src/band_extract.cpp            # 波段提取/拆分
        src/extract_import_points.cpp   # 从栅格图像中提取关键点
        src/quadtree.cpp                # 栅格图基于特定规则生成四叉树
        src/jpg_to_png.cpp              # jpg栅格图转png栅格图
        #获取图像在某条直线上的值
        )
target_link_libraries(gdal_tool_raster PRIVATE GDAL::GDAL)
target_link_libraries(gdal_tool_raster PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(gdal_tool_raster PRIVATE argparse::argparse)
target_link_libraries(gdal_tool_raster PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(gdal_tool_raster PRIVATE nlohmann_json::nlohmann_json)
set(EXE_LIST ${EXE_LIST} gdal_tool_raster)

add_executable(gdal_tool_vector
        src/main_vector.cpp
        src/datatype.h
        src/datatype.cpp
        src/vector_include.h
        src/point_with_shapefile.cpp            # 判断点是否在shp文件中
        src/create_polygon_shapefile.cpp        # 创建多边形shp文件
        src/create_point_shapefile.cpp          # 创建点shp文件  创建3维点shp文件
        src/polygen_with_shapefile.cpp          # 判断多边形与shp是否相交
        )
target_link_libraries(gdal_tool_vector PRIVATE GDAL::GDAL)
target_link_libraries(gdal_tool_vector PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(gdal_tool_vector PRIVATE argparse::argparse)
set(EXE_LIST ${EXE_LIST} gdal_tool_vector)


add_executable(gdal_tool_insar
        src/main_insar.cpp
        src/datatype.h
        src/datatype.cpp
        src/insar_include.h
        src/goldstein.cpp                   # goldstein 滤波        
        src/goldstein_zhao.cpp              # goldstein-zhao 滤波
        src/goldstein_baran.cpp             # goldstrin-baran 滤波
        src/pseudo_correlation.cpp          # 伪相干性计算
        )
target_link_libraries(gdal_tool_insar PRIVATE GDAL::GDAL)
target_link_libraries(gdal_tool_insar PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(gdal_tool_insar PRIVATE argparse::argparse)
target_link_libraries(gdal_tool_insar PRIVATE FFTW3::fftw3f)
target_link_libraries(gdal_tool_insar PRIVATE OpenMP::OpenMP_CXX)
set(EXE_LIST ${EXE_LIST} gdal_tool_insar)

#debug config 测试
add_executable(debug_config_test src/debug_config_test.cpp)


#创建delaunay
add_executable(create_delaunay src/CreateDelaunay.cpp src/datatype.h src/datatype.cpp)
target_link_libraries(create_delaunay PRIVATE GDAL::GDAL)
target_link_libraries(create_delaunay PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(create_delaunay PRIVATE argparse::argparse)
set(EXE_LIST ${EXE_LIST} create_delaunay)

# duqu EGM2008文件, 并写出
add_executable(read_egm2008 src/read_egm2008.cpp src/datatype.cpp)
target_include_directories( read_egm2008 
                                INTERFACE 
                                ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(read_egm2008 PRIVATE GDAL::GDAL)
target_link_libraries(read_egm2008 PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(read_egm2008 PRIVATE fmt::fmt)
target_link_libraries(read_egm2008 PRIVATE argparse::argparse)
set(EXE_LIST ${EXE_LIST} read_egm2008)

# 同一坐标系统的分块数据拼接
add_executable(unified_geoimage_merging src/unified_GeoImage_merging.cpp src/datatype.h src/datatype.cpp)
target_link_libraries(unified_geoimage_merging PRIVATE GDAL::GDAL)
target_link_libraries(unified_geoimage_merging PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
if(OpenMP_FOUND)
        target_link_libraries(unified_geoimage_merging PRIVATE OpenMP::OpenMP_CXX)
else()
        message(FATAL_ERROR "openmp not found!")
endif()
set(EXE_LIST ${EXE_LIST} unified_geoimage_merging)

#获取图像在某条直线上的值
add_executable(get_image_value_in_line src/get_image_value_in_line.cpp src/datatype.h src/datatype.cpp)
target_link_libraries(get_image_value_in_line PRIVATE GDAL::GDAL)
target_link_libraries(get_image_value_in_line PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(get_image_value_in_line PRIVATE fmt::fmt)
set(EXE_LIST ${EXE_LIST} get_image_value_in_line)

#GDAL虚拟文件系统测试
add_executable(virtual_files_system_test src/virtual_files_system_test.cpp src/datatype.h src/datatype.cpp)
target_link_libraries(virtual_files_system_test PRIVATE GDAL::GDAL)
target_link_libraries(virtual_files_system_test PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(virtual_files_system_test PRIVATE fmt::fmt)
# set(EXE_LIST ${EXE_LIST} virtual_files_system_test)



#LandSAR 将存储PS点的tif转换为栅格图tif
add_executable(landsar_ps_points_transto_tif src/landsar_ps_points_transto_tif.cpp)
target_link_libraries(landsar_ps_points_transto_tif PRIVATE GDAL::GDAL)
# set(EXE_LIST ${EXE_LIST} landsar_ps_points_transto_tif)


# add "cmake.configureSettings": {"CMAKE_BUILD_TYPE":"${buildType}"} in setting.json, when ${CMAKE_BUILD_TYPE} is ""
message(STATUS "cmake build type: " ${CMAKE_BUILD_TYPE})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE})
file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/proj.db ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/proj.db ONLY_IF_DIFFERENT)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/data)
file(COPY_FILE 
        ${CMAKE_CURRENT_LIST_DIR}/data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE 
        ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE ONLY_IF_DIFFERENT)

file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/scripts/add_path_template.ps1 ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/add_path_template.ps1 ONLY_IF_DIFFERENT)

# INSTALL

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)

# message("exe_list:" "${EXE_LIST}")
install(TARGETS ${EXE_LIST}
        CONFIGURATIONS Release
        RUNTIME DESTINATION binary
        LIBRARY DESTINATION library
        ARCHIVE DESTINATION library
)

# GET_RUNTIME_DEPENDENCIES 可以导入第三方库, 但没有摸清楚怎么使用, 从网上找了个临时替代的方法, 如下所示
file(GLOB DLLS "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/*.dll") # 假设DLLs在构建目录中
install(FILES ${DLLS} DESTINATION binary CONFIGURATIONS Release)
# message("DLLS: ${DLLS}")

install(FILES ${CMAKE_CURRENT_LIST_DIR}/proj.db DESTINATION binary CONFIGURATIONS Release)
install(FILES ${CMAKE_CURRENT_LIST_DIR}/data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE DESTINATION binary/data CONFIGURATIONS Release)
# install(FILES ${CMAKE_CURRENT_LIST_DIR}/vcruntime140d.dll ${CMAKE_CURRENT_LIST_DIR}/vcruntime140_1d.dll DESTINATION binary/vc CONFIGURATIONS Release)

file(GLOB scripts "scripts/*.ps1" "scripts/*.bat") # 假设DLLs在构建目录中
install(FILES ${scripts} DESTINATION binary CONFIGURATIONS Release)

file(GLOB vcruntime "vcruntime/*.dll" ) # 假设DLLs在构建目录中
install(FILES ${vcruntime} DESTINATION binary CONFIGURATIONS Release)
# message("scripts: ${scripts}")