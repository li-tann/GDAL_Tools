import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal
import os
import sys
import io

'''输出(打印)utf-8字符串'''
sys.stdout=io.TextIOWrapper(sys.stdout.buffer,encoding='utf8')

def is_polygen_Intersects_with_shps(shp_rootpath, geometry_in):
    '''
    判断shp_rootpath中是否存在与geometry相交的shp \n
    shp_rootpath: shp的根目录 \n
    geometry 从矢量文件中提取出的geometry
    '''
    osr.UseExceptions()
    gdal.SetConfigOption("GDAL_FILENAME_IS_UTF8", "YES")

    for root, dirs, files in os.walk(shp_rootpath):
        for name in files:
            if not str(name).endswith('.shp'):
                continue
            file_path = os.path.join(root, name)
            
            # try:
            #     ds=ogr.Open(file_path)
            #     layer:ogr.Layer = ds.GetLayer(0)
            #     feature_count = layer.GetFeatureCount()

            #     for i in range(feature_count):
            #         feature = layer.GetFeature(i)
            #         geometry:ogr.Geometry = feature.GetGeometryRef()

            #         if(geometry.Intersects(geometry_in)):
            #             return True
            # except:
            #     pass
            ds=ogr.Open(file_path)
            layer:ogr.Layer = ds.GetLayer(0)
            feature_count = layer.GetFeatureCount()

            for i in range(feature_count):
                feature = layer.GetFeature(i)
                geometry:ogr.Geometry = feature.GetGeometryRef()

                if(geometry.Intersects(geometry_in)):
                    return True
                
    return False



if __name__ == "__main__":
    osr.UseExceptions()
    gdal.SetConfigOption("GDAL_FILENAME_IS_UTF8", "YES")

    '''一半在shp中,一般在shp外'''
    slc_shp = "C:/Users/lenovo/Desktop/shp_topological/in/LT1A_MONO_KRN_STRIP1_008583_E95.3_N28.3_20230825_SLC_HH_S2A_0000187554_SLC.shp"   
    ''' 完全在shp外'''
    # slc_shp = "C:/Users/lenovo/Desktop/shp_topological/out/LT1A_MONO_KRN_STRIP1_008583_E95.7_N29.4_20230825_SLC_HH_S2A_0000187559_SLC.shp"
    '''完全包含在shp中'''
    # slc_shp = "C:/Users/lenovo/Desktop/shp_topological/in_2/ceshi.shp"

    ds_in=ogr.Open(slc_shp)
    lyr = ds_in.GetLayer(0)
    ftr:ogr.Feature = lyr.GetFeature(0)

    polygen = ftr.GetGeometryRef()
    print(polygen.GetArea())

    b = is_polygen_Intersects_with_shps("C:/Users/lenovo/Desktop/shp_topological/target", ftr.GetGeometryRef())
    if(b):
        print("feature is Intersects with shps")
    else:
        print("feature is not Intersects with shps")




