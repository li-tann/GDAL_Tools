from landsar_slc_to_shp import create_polygon_shapefile, from_SLC_XML_import_corner, corner_list_to_polygen_str, from_SLC_XML_import_productid
from vector_topological_test import is_polygen_Intersects_with_shps
import os
import sys
import io
import osgeo.ogr as ogr
import osgeo.osr as osr
import osgeo.gdal as gdal

# '''输出(打印)utf-8字符串'''
# sys.stdout=io.TextIOWrapper(sys.stdout.buffer,encoding='utf8')

if __name__ == "__main__":
    '''
    1.遍历xml, 生成geometry
    2.geometry与target_shapefiles比较, 判断交互关系
    3.如果相交, 则将geometry输出到指定目录
    '''
    

    osr.UseExceptions()
    gdal.SetConfigOption("GDAL_FILENAME_IS_UTF8", "YES")

    xml_rootpath = 'D:/1_Data/lutan_xml_china_landsar_2'
    # xml_rootpath = 'C:/Users/lenovo/Desktop/shp_topological/in_xml_test'
    shp_rootpath = 'C:/Users/lenovo/Desktop/shp_topological/target'

    output_shp_rootpath = 'C:/Users/lenovo/Desktop/shp_topological/in_shp'

    idx = idx2 = 0
    for root, dirs, files in os.walk(xml_rootpath):
        for name in files:
            if not str(name).endswith('.xml'):
                continue
            idx += 1
            # print(idx, name)
            file_path = os.path.join(root, name)
            shp_path = os.path.join(output_shp_rootpath, os.path.splitext(name)[0] + '.shp')
            if(os.path.exists(shp_path)):
                continue
            
            cornerlist = from_SLC_XML_import_corner(file_path)
            polygen_str = corner_list_to_polygen_str(cornerlist)
            productid = from_SLC_XML_import_productid(file_path)

            polygen:ogr.Geometry = ogr.CreateGeometryFromWkt(polygen_str)

            if not(is_polygen_Intersects_with_shps(shp_rootpath, polygen)):
                print(idx2,'/', idx, shp_path, flush=True)
                continue
            
            idx2 += 1
            # print("file_path:", file_path)
            # print("shp_path:", shp_path)

            create_polygon_shapefile(shp_path, productid, polygen_str)
            print(idx2,'/', idx, shp_path, flush=True)
