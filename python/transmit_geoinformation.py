from sys import argv
from osgeo import gdal

script=""
refGeo=""
transTo=""

if(len(argv)>=3):
    script, refGeo, transTo = argv
else:
    refGeo = "D:/1_Data/L-SAR_TEST/dinsar_mono_cut2/04_flt/LT1AA_20230321_20230329.DEM_RectSeg.tif"
    transTo = "c:/Users/lenovo/Desktop/test.tif"

# print("script: ",script)
# print("refGeo: ",refGeo)
# print("transTo: ",transTo)
# print("len(argv): ",len(argv))

gdal.UseExceptions()

ds_ref: gdal.Dataset = gdal.Open(refGeo, gdal.GA_ReadOnly)
ds_transto: gdal.Dataset = gdal.Open(transTo,gdal.GA_Update)

ds_transto.SetGeoTransform(ds_ref.GetGeoTransform())
ds_transto.SetProjection(ds_ref.GetProjectionRef())

ds_ref = None
ds_transto = None

print("geoinformation(geoTransfrom + projection) transmit success.")

