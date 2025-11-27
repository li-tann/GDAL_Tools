from osgeo import gdal

'''示例中, 将一个wgs84坐标系的DEM转换到CGCS2000/GK19坐标系统中'''

gdal.UseExceptions()

input_dem = 'ALPSMLC30_N033E109_DSM.tif'
output_dem = 'ALPSMLC30_N033E109_DSM.4497.CSCG2000_GK19_108_114.tif'
epsg = 4497  # 例如，转换为WGS84坐标系

src = gdal.Open(input_dem)
gdal.Warp(output_dem, src, dstSRS=f'EPSG:{epsg}')