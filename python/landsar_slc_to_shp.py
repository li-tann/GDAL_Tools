import osgeo.ogr as ogr
import osgeo.osr as osr
import xml.etree.ElementTree as ET
import os

'''
矢量数据的交集判断
'''

def create_polygon_shapefile(vector_path, layer_name, geometry_str):
    '''
    创建面状的vector
    '''

    osr.UseExceptions()

    driver = ogr.GetDriverByName('ESRI Shapefile')
    data_source = driver.CreateDataSource(vector_path)

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)


    layer = data_source.CreateLayer(layer_name, srs, ogr.wkbPolygon)

    field_name = ogr.FieldDefn("Name", ogr.OFTString)
    field_name.SetWidth(14)
    layer.CreateField(field_name)

    feature = ogr.Feature(layer.GetLayerDefn())
    feature.SetField("Name", "slc")

    polygen = ogr.CreateGeometryFromWkt(geometry_str)
    feature.SetGeometry(polygen)
    layer.CreateFeature(feature)

    feature = None
    data_source = None

def from_SLC_XML_import_corner_origin(xmlpath):
    '''
    读取LandSAR格式的XML文件, 获取角点的经纬度信息, 打印成字符串.
    字符串格式可以直接使用CreateGeometryFromWkt(str)'生成ogr.geometry
    '''
    tree = ET.parse(xmlpath)
    root = tree.getroot()

    commonModule = root.find('SARParametersofCommonModule')
    topleft = commonModule.find('sceneLeftTopCoord')
    tl_lat = float(topleft.find('lat').text)
    tl_lon = float(topleft.find('lon').text)
    wkb = "{:.3f} {:.3f},".format(tl_lon, tl_lat)

    topright = commonModule.find('sceneRightTopCoord')
    tr_lat = float(topright.find('lat').text)
    tr_lon = float(topright.find('lon').text)
    wkb += "{:.3f} {:.3f},".format(tr_lon, tr_lat)

    downright = commonModule.find('sceneRightBottomCoord')
    dr_lat = float(downright.find('lat').text)
    dr_lon = float(downright.find('lon').text)
    wkb += "{:.3f} {:.3f},".format(dr_lon, dr_lat)

    downleft = commonModule.find('sceneLeftBottomCoord')
    dl_lat = float(downleft.find('lat').text)
    dl_lon = float(downleft.find('lon').text)
    wkb += "{:.3f} {:.3f},".format(dl_lon, dl_lat)

    wkb += "{:.3f} {:.3f}".format(tl_lon, tl_lat)
    return 'POLYGON({})'.format(wkb)

def from_SLC_XML_import_corner(xmlpath):
    '''
    读取LandSAR格式的XML文件, 获取角点的经纬度信息, 以list形式输出
    '''
    tree = ET.parse(xmlpath)
    root = tree.getroot()

    corner_list = []

    commonModule = root.find('SARParametersofCommonModule')
    topleft = commonModule.find('sceneLeftTopCoord')
    tl_lat = float(topleft.find('lat').text)
    tl_lon = float(topleft.find('lon').text)
    corner_list.append((tl_lon, tl_lat))

    topright = commonModule.find('sceneRightTopCoord')
    tr_lat = float(topright.find('lat').text)
    tr_lon = float(topright.find('lon').text)
    corner_list.append((tr_lon, tr_lat))

    downright = commonModule.find('sceneRightBottomCoord')
    dr_lat = float(downright.find('lat').text)
    dr_lon = float(downright.find('lon').text)
    corner_list.append((dr_lon, dr_lat))

    downleft = commonModule.find('sceneLeftBottomCoord')
    dl_lat = float(downleft.find('lat').text)
    dl_lon = float(downleft.find('lon').text)
    corner_list.append((dl_lon, dl_lat))

    return corner_list

def corner_list_to_polygen_str(corner_list):
    '''
    读取角点列表, 转换成polygen字符串格式。\n
    可以直接使用 CreateGeometryFromWkt(str) 生成 ogr.geometry
    '''
    poly_str = '('
    for corner in corner_list:
        poly_str += "{:.3f} {:.3f},".format(corner[0], corner[1])
    
    poly_str += "{:.3f} {:.3f})".format(corner_list[0][0], corner_list[0][1])

    return 'POLYGON({})'.format(poly_str)



def from_SLC_XML_import_productid(xmlpath):
    '''
    读取product ID, 以字符串形式输出
    '''
    tree = ET.parse(xmlpath)
    root = tree.getroot()

    commonModule = root.find('SARParametersofCommonModule')
    return commonModule.find('productID').text



if __name__ == "__main__":

    # test_xmlpath='D:/1_Data/lutan_xml_china_landsar/LT1A_MONO_KRN_STRIP1_008472_E83.0_N47.6_20230818_SLC_HH_S2A_0000181967_SLC.xml'

    # cornerlist = from_SLC_XML_import_corner(test_xmlpath)
    # print("corner_list:", cornerlist)
    # polygen_str = corner_list_to_polygen_str(cornerlist)
    # print("polygen_str:", polygen_str)
    # productid = from_SLC_XML_import_productid(test_xmlpath)
    # print("productid:", productid)

    # create_polygon_shapefile("{}.shp".format(productid), productid, polygen_str)


    if(False):
        '''
        遍历, 耗时100s, 不建议轻易使用
        '''
        idx = 0
        xml_rootpath = "D:/1_Data/lutan_xml_china_landsar"
        shp_rootpath = "D:/1_Data/lutan_shp_china"
        for root, dirs, files in os.walk(xml_rootpath):
            for name in files:
                if not str(name).endswith('.xml'):
                    continue
                file_path = os.path.join(root, name)
                # shp_path = os.path.join(shp_rootpath, os.path.splitext(name)[0] + '.shp')
                # print(idx, file_path)
                # print(idx, shp_path)
                
                # if(idx > 5):
                #     exit()
                cornerlist = from_SLC_XML_import_corner(file_path)
                polygen_str = corner_list_to_polygen_str(cornerlist)
                productid = from_SLC_XML_import_productid(file_path)

                shp_path = os.path.join(shp_rootpath, os.path.splitext(name)[0] + '.shp')
                create_polygon_shapefile(shp_path, productid, polygen_str)

                idx+=1
                print(idx, productid)
        exit()

