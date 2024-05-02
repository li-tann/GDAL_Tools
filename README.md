# GDAL_Tools

Collection of executable, which dependent on GDAL (and spdlog)

写一些基于GDAL的工具集，依赖GDAL和spdlog两个库

随便写写，实际使用时难免会有很多bug

## Module

### binary_img_to_tif

二进制文件转tif图，目前已测试果的数据格式仅仅fcomplex

### tif_to_binary_img

tif图转换为二进制文件

### create delaunay

提供二维点文件, 创建delaunay三角网，输出所有三角形的坐标

### determine topological relationship

计算一个点与shp的拓扑关系，输入一个点平面坐标，一个shp文件，文字形式输出点与shp文件的关系(in or out)

### histogram stretch

对影像进行百分比拉伸计算，去除两端的噪声

### nan trans to

将图像中的nan值统一转换为一个指定的数值

### set nodata value

将图像中某一个值标记为nodata，从而使大部分地学看图软件(envi, arcmap, ...)不显示nodata值, 实现去除编码后黑色无数据区域的效果

### statistics

对影像的某个波段(RasterBand, 波段, 图层, 通道,...)进行数值统计，输出该波段数据的最值、均值和方差

### egm2008

通过egm2008，输出单点经纬度，或经纬度文件，或带地理坐标的DEM文件，输入小端存储（*_SE）的EGM2008文件,输出对应点或范围的高程异常值

### vrt_trans

vrt格式的数据转换为tif格式，或tif格式的数据转换为vrt格式

### over_resample

基于GDAL的Warp，对影像进行重采样, 并以tif格式输出

### transmit_geoinformation

把影像A的GeoTransform和ProjectRef写到影像B内

### unified_GeoImage_merging

统一坐标系统的影像的拼接，例如全球分块的DEM文件。

以DEM为例，输入所有DEM文件所在的<**根目录**>（允许多层级迭代），再输入一个目标区域的<**shp文件**>，选择拼接方法

拼接方法有两种，

minimum表示只提取与shp有交集的DEM数据进行拼接，当shp文件不规则时可能会出现拼接结果边角缺少数据的情况；

maximum表示提取所有在shp文件四至范围内的DEM数据进行拼接，所以拼接结果将会是一个完整的矩形（除非缺少DEM数据）但也会失去shp的形状特点；

程序默认<**根目录**>内所有DEM都在统一坐标系统内，并且分辨率完全相同，即没有进行重采样等操作，而是直接计算每个待拼接DEM在拼接后DEM的起始位置，直接将所有像素值“平移”过去。

当<**根目录**>内存在两种及以上坐标系统的影像且均被用于拼接，或与shp文件坐标系统不相同时，可能会出现输出影像尺寸离谱、无法正确判断相交情况等各种奇奇怪怪的异常问题。

考虑到<**根目录**>中可能存在除DEM之外的无效数据，程序内也提供了基于文件名称的正则表达式筛选条件（可选），仅将通过了正则筛选的数据作为拼接待选数据。

TDM1_DEM的文件结构为：

```powershell
D:.
│  demProduct.xsd
│  generalHeader.xsd
│  TDM1_DEM__30_N39E112.xml
│  types_inc.xsd
│
├─AUXFILES
│      TDM1_DEM__30_N39E112_AM2.tif
│      TDM1_DEM__30_N39E112_AMP.tif
│      TDM1_DEM__30_N39E112_COM.tif
│      TDM1_DEM__30_N39E112_COV.tif
│      TDM1_DEM__30_N39E112_HEM.tif
│      TDM1_DEM__30_N39E112_LSM.tif
│      TDM1_DEM__30_N39E112_WAM.tif
│
├─DEM
│      TDM1_DEM__30_N39E112_DEM.tif
│
└─PREVIEW
        TDM1_DEM__30_N39E112.kml
        TDM1_DEM__30_N39E112_AM2.kml
        TDM1_DEM__30_N39E112_AM2_QL.tif
        TDM1_DEM__30_N39E112_AMP.kml
        TDM1_DEM__30_N39E112_AMP_QL.tif
        TDM1_DEM__30_N39E112_COM.kml
        TDM1_DEM__30_N39E112_COM_LEGEND_QL.png
        TDM1_DEM__30_N39E112_COM_QL.tif
        TDM1_DEM__30_N39E112_COV.kml
        TDM1_DEM__30_N39E112_COV_LEGEND_QL.png
        TDM1_DEM__30_N39E112_COV_QL.tif
        TDM1_DEM__30_N39E112_DEM.kml
        TDM1_DEM__30_N39E112_DEM_ABS_QL.tif
        TDM1_DEM__30_N39E112_DEM_LEGEND_QL.png
        TDM1_DEM__30_N39E112_DEM_QL.tif
        TDM1_DEM__30_N39E112_DEM_THUMB_QL.png
        TDM1_DEM__30_N39E112_DEM_WAM_ABS_QL.tif
        TDM1_DEM__30_N39E112_DEM_WAM_QL.tif
        TDM1_DEM__30_N39E112_HEM.kml
        TDM1_DEM__30_N39E112_HEM_LEGEND_QL.png
        TDM1_DEM__30_N39E112_HEM_QL.tif
        TDM1_DEM__30_N39E112_LSM.kml
        TDM1_DEM__30_N39E112_LSM_LEGEND_QL.png
        TDM1_DEM__30_N39E112_LSM_QL.tif
        TDM1_DEM__30_N39E112_WAM.kml
        TDM1_DEM__30_N39E112_WAM_LEGEND_QL.png
        TDM1_DEM__30_N39E112_WAM_QL.tif
```

TDM_DEM文件中有很多tif数据，但只有DEM数据的后缀名称是“xxxx_DEM.tif”，所以使用正则表达式`.*DEM.tif$`可以从中筛选出DEM数据。

### create_shapfile

输入一个记录经纬度信息的shapfile文件

## 更新日志

### 2024.05.03

增加了argparse库, 使终端的命令输入模式更加规范。

删除了部分测试项的功能模块, 将部分与栅格相关的模块整合到gdal_tool_raster中，将部分与矢量文件相关的模块整合到gdal_tool_vector中。

部分特殊的模块，如read_egm2008, geoimage_merging等, 暂时已单独模块的形式存在。

gdal_tool_raster和gdal_tool_vector两个集合，可以使用`gdal_tool_raster -h`指令查看使用方法，使用`gdal_tool_vector \[sub_module\] -h`指令查看二级模块的使用方法。

如:

（举例说明）
