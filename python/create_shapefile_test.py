import osgeo.ogr as ogr
import osgeo.osr as osr

'''
创建shapefile
'''


if __name__ == "__main__":

    osr.UseExceptions()

    driver = ogr.GetDriverByName("ESRI Shapefile")
    data_source = driver.CreateDataSource("C:/Users/lenovo/Desktop/shp_topological/in_2/ceshi.shp")

    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)

    #ogr.wkbPoint 点
    #ogr.wkbLineString 线
    #ogr.wkbMultiPolygon 面
    layer = data_source.CreateLayer("ceshi", srs, ogr.wkbPolygon)

    field_name = ogr.FieldDefn("Name", ogr.OFTString)
    field_name.SetWidth(14)
    layer.CreateField(field_name)

    feature = ogr.Feature(layer.GetLayerDefn())
    feature.SetField("Name", "ceshi")
    # feature.SetField("data", "1.2")

    '''
    其中各要素格式如下：
    POINT(6 10)
    LINESTRING(3 4,10 50,20 25)
    POLYGON((1 1,5 1,5 5,1 5,1 1),(2 2,2 3,3 3,3 2,2 2))
    MULTIPOINT(3.5 5.6, 4.8 10.5)
    MULTILINESTRING((3 4,10 50,20 25),(-5 -8,-10 -8,-15 -4))
    MULTIPOLYGON(((1 1,5 1,5 5,1 5,1 1),(2 2,2 3,3 3,3 2,2 2)),((6 3,9 2,9 4,6 3)))
    GEOMETRYCOLLECTION(POINT(4 6),LINESTRING(4 6,7 10))
    POINT ZM (1 1 5 60)
    POINT M (1 1 80)
    '''

    wkt = 'POLYGON((102.7 26.7,103 26.7,103 27,102.7 27,102.7 26.7))'
    multiPolygen = ogr.CreateGeometryFromWkt(wkt)
    feature.SetGeometry(multiPolygen)
    layer.CreateFeature(feature)

    feature = None
    data_source = None