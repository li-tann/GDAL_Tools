# execs_dependent_on_gdal

Collection of executable, which dependent on GDAL (and spdlog)

写一些基于GDAL的工具集，依赖GDAL和spdlog两个库

随便写写，实际使用时难免会有很多bug

## nan_point_fill_in.cpp

把影像的nan值转换为指定数值

## set_nodata_value.cpp

给影像添加NoDataValue字段，使用arcgis显示时无黑边

## binary_img_to_tif.cpp

二进制文件转tif图，目前已测试果的数据格式仅仅fcomplex
