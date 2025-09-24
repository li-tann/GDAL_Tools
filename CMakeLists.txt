cmake_minimum_required(VERSION 3.0.0)
project(execs VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_CXX_STANDARD_REQUIRED on)

include(CTest)
enable_testing()

message("buildType: ${CMAKE_BUILD_TYPE}")

# CMAKE_RUNTIME_OUTPUT_DIRECTORY 设置方法: 
# windows下, exe的生产地址默认为：${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/xxxx.exe
#                        或 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/xxxx.exe
# 而linux下, exe(可执行文件)的生成地址默认为: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/xxxx.exe
#                        或 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/xxxx.exe
# 所以最后决定使用 CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE 和 CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release/library)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR}/Release/library)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug/library)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR}/Debug/library)

find_package(GDAL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
find_package(FFTW3f CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
set(nlohmann_json_IMPLICIT_CONVERSIONS OFF)
find_package(triangle CONFIG REQUIRED)

# find_package(Boost REQUIRED COMPONENTS graph)

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
        src/triangle_network.cpp        # 调用trignaleLib执行构网操作, 对输入输出进行修改
        src/geotransform_trans.cpp      # 六参数与坐标的转换
        #获取图像在某条直线上的值
        )
target_link_libraries(gdal_tool_raster PRIVATE GDAL::GDAL)
target_link_libraries(gdal_tool_raster PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(gdal_tool_raster PRIVATE argparse::argparse)
target_link_libraries(gdal_tool_raster PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(gdal_tool_raster PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(gdal_tool_raster PRIVATE triangle::triangleLib)

set(EXE_LIST ${EXE_LIST} gdal_tool_raster)

add_executable(gdal_tool_vector
        src/main_vector.cpp
        src/datatype.h
        src/datatype.cpp
        src/vector_include.h
        src/point_with_shapefile.cpp            # 判断点是否在shp文件中
        src/create_polygon_shapefile.cpp        # 创建多边形shp文件
        src/create_point_shapefile.cpp          # 创建点shp文件  创建3维点shp文件
        src/create_linstring_shapefile.cpp      # 创建多端线shp
        src/polygen_with_shapefile.cpp          # 判断多边形与shp是否相交
        src/shapefile_points_dilution.cpp       # 点shp文件抽稀
        # src/find_nearest_point_from_shp.cpp     # 从点shp文件中搜索与指定目标最接近的点
        )
target_link_libraries(gdal_tool_vector PRIVATE GDAL::GDAL)
target_link_libraries(gdal_tool_vector PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(gdal_tool_vector PRIVATE argparse::argparse)
# target_link_libraries(gdal_tool_vector PRIVATE Boost::boost Boost::graph)
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


# GDAL MEM:::POINTER功能测试
add_executable(mem_pointer src/mem_pointer_test.cpp)
target_link_libraries(mem_pointer PRIVATE GDAL::GDAL)
target_link_libraries(mem_pointer PRIVATE fmt::fmt)

# 读写hdf5文件-测试
find_package(hdf5 CONFIG REQUIRED)

add_executable(io_hdf5 src/temp_hdf5.cpp)
target_link_libraries(io_hdf5 PRIVATE fmt::fmt)
target_link_libraries(io_hdf5 PRIVATE GDAL::GDAL)
target_link_libraries(io_hdf5 PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(io_hdf5 PRIVATE hdf5::hdf5-shared hdf5::hdf5_hl-shared hdf5::hdf5_cpp-shared hdf5::hdf5_hl_cpp-shared)


 # float32与float16相互转换
find_path(HALF_INCLUDE_DIRS "half.hpp")
if(HALF_INCLUDE_DIRS)
add_executable(float32_16
                src/float32_to_float16.cpp 
                src/datatype.h
                src/datatype.cpp)
target_link_libraries(float32_16 PRIVATE fmt::fmt)
target_link_libraries(float32_16 PRIVATE GDAL::GDAL)
target_link_libraries(float32_16 PRIVATE spdlog::spdlog spdlog::spdlog_header_only)

target_include_directories(float32_16 PRIVATE ${HALF_INCLUDE_DIRS})

set(EXE_LIST ${EXE_LIST} float32_16)
else()
message(STATUS "there is not found half.hpp")
endif()

# add "cmake.configureSettings": {"CMAKE_BUILD_TYPE":"${buildType}"} in setting.json, when ${CMAKE_BUILD_TYPE} is ""
message(STATUS "cmake build type: " ${CMAKE_BUILD_TYPE})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE})
file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/proj.db ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/proj.db ONLY_IF_DIFFERENT)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/data)
file(COPY_FILE 
        ${CMAKE_CURRENT_LIST_DIR}/data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE 
        ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE ONLY_IF_DIFFERENT)

file(COPY_FILE ${CMAKE_CURRENT_LIST_DIR}/scripts/add_path_template.ps1 ${CMAKE_CURRENT_LIST_DIR}/build/${CMAKE_BUILD_TYPE}/add_path_template.ps1 ONLY_IF_DIFFERENT)


# 如果是linux系统, 则需要设置rpath, 并且创建lib文件夹(暂未实现) 
if(UNIX)
message("it's UNIX")
message("EXE_LIST:" "${EXE_LIST}")
foreach(EXE_NAME IN LISTS EXE_LIST)
        set_target_properties(${EXE_NAME} PROPERTIES
                INSTALL_RPATH "\$ORIGIN/lib"
                BUILD_WITH_INSTALL_RPATH TRUE  # 构建时即使用 INSTALL_RPATH
        )
endforeach()
else()
message("it't not UNIX")
endif()



# INSTALL

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install)

# message("exe_list:" "${EXE_LIST}")
install(TARGETS ${EXE_LIST}
        CONFIGURATIONS Release
        RUNTIME DESTINATION binary
        LIBRARY DESTINATION library
        ARCHIVE DESTINATION library
)

# 仅在windows系统下 需要拷贝*.dll, *.bat/ps1 到安装目录
if(WIN32)
# GET_RUNTIME_DEPENDENCIES 可以导入第三方库, 但没有摸清楚怎么使用, 从网上找了个临时替代的方法, 如下所示
file(GLOB DLLS "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/*.dll") # 假设DLLs在构建目录中
install(FILES ${DLLS} DESTINATION binary CONFIGURATIONS Release)
# message("DLLS: ${DLLS}")

file(GLOB scripts "scripts/*.ps1" "scripts/*.bat") # 假设DLLs在构建目录中
install(FILES ${scripts} DESTINATION binary CONFIGURATIONS Release)

file(GLOB vcruntime "vcruntime/*.dll" ) # 假设DLLs在构建目录中
install(FILES ${vcruntime} DESTINATION binary CONFIGURATIONS Release)
# message("scripts: ${scripts}")
endif(WIN32)

install(FILES ${CMAKE_CURRENT_LIST_DIR}/proj.db DESTINATION binary CONFIGURATIONS Release)
install(FILES ${CMAKE_CURRENT_LIST_DIR}/data/Und_min10x10_egm2008_isw=82_WGS84_TideFree_SE DESTINATION binary/data CONFIGURATIONS Release)
# install(FILES ${CMAKE_CURRENT_LIST_DIR}/vcruntime140d.dll ${CMAKE_CURRENT_LIST_DIR}/vcruntime140_1d.dll DESTINATION binary/vc CONFIGURATIONS Release)

