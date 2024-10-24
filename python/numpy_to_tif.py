import numpy as np
from osgeo import gdal



data = np.zeros((12,12),dtype=np.byte)
data[1][1]=1
data[1][5]=1
data[5][4]=1
data[7][11]=1
data[9][9]=1
data[10][10]=1

print(data)

gdal.UseExceptions()
driver: gdal.Driver = gdal.GetDriverByName('gtiff')
ds_out: gdal.Dataset = driver.Create("d:/mask.tif", 12, 12, 1, gdal.GDT_Byte)
band_out: gdal.Band = ds_out.GetRasterBand(1)
band_out.WriteArray(data)

ds_out.FlushCache()