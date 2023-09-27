# execs_dependent_on_gdal

Collection of executable, which dependent on GDAL (and spdlog)

写一些基于GDAL的工具集，依赖GDAL和spdlog两个库

随便写写，实际使用时难免会有很多bug

## binary_img_to_tif

二进制文件转tif图，目前已测试果的数据格式仅仅fcomplex

## tif_to_binary_img

tif图转换为二进制文件

## create delaunay

提供二维点文件, 创建delaunay三角网，输出所有三角形的坐标

## determine topological relationship

计算一个点与shp的拓扑关系，输入一个点平面坐标，一个shp文件，文字形式输出点与shp文件的关系(in or out)

## histogram stretch

对影像进行百分比拉伸计算，去除两端的噪声

## nan trans to

将图像中的nan值统一转换为一个指定的数值

## set nodata value

将图像中某一个值标记为nodata，从而使大部分地学看图软件(envi, arcmap, ...)不显示nodata值, 实现去除编码后黑色无数据区域的效果

## statistics

对影像的某个波段(RasterBand, 波段, 图层, 通道,...)进行数值统计，输出该波段数据的最值、均值和方差

## print_sar_image

打印sar影像(支持复数格式), 设置转换格式phase / power(仅复数数据有效), 最值筛选方式 auto / manual, 颜色表, 并输出到指定地址下

## egm2008

通过egm2008，输出单点经纬度，或经纬度文件，或带地理坐标的DEM文件，输入小端存储（*_SE）的EGM2008文件,输出对应点或范围的高程异常值

## vrt_test

读或写一个vrt文件

## vrt_trans

vrt格式的数据转换为tif格式，或tif格式的数据转换为vrt格式

## over_resample

基于GDAL的Warp，对影像进行重采样, 并以tif格式输出

## transmit_geoinformation

把影像A的GeoTransform和ProjectRef写到影像B内
