from osgeo import ogr
import os


def clip_shp_1(input_shp_path, clip_shp_path, output_shp_path):
    '''裁剪clip_shp范围内的input_shp, 输出为outout_shp'''

    ogr.UseExceptions()

    # 打开输入数据源
    in_driver = ogr.GetDriverByName('ESRI Shapefile')
    in_datasource = in_driver.Open(input_shp_path, 0)
    if in_datasource is None:
        raise FileNotFoundError(f"无法打开输入文件: {input_shp_path}")
    in_layer = in_datasource.GetLayer()

    # 打开裁剪数据源
    clip_driver = ogr.GetDriverByName('ESRI Shapefile')
    clip_datasource = clip_driver.Open(clip_shp_path, 0)
    if clip_datasource is None:
        raise FileNotFoundError(f"无法打开裁剪文件: {clip_shp_path}")
    clip_layer = clip_datasource.GetLayer()

    # 创建输出数据源
    out_driver = ogr.GetDriverByName('ESRI Shapefile')
    if out_driver is None:
        raise Exception('ESRI Shapefile 驱动不可用')
    if os.path.exists(output_shp_path):
        out_driver.DeleteDataSource(output_shp_path)
    out_datasource = out_driver.CreateDataSource(output_shp_path)
    out_layer = out_datasource.CreateLayer(
        in_layer.GetName(),
        srs=in_layer.GetSpatialRef(),
        geom_type=in_layer.GetGeomType()
    )

    # 复制输入图层的字段定义到输出图层
    in_layer_defn = in_layer.GetLayerDefn()
    for i in range(0, in_layer_defn.GetFieldCount()):
        field_defn = in_layer_defn.GetFieldDefn(i)
        out_layer.CreateField(field_defn)

    # 进行裁剪操作
    for in_feature in in_layer:
        in_geom = in_feature.GetGeometryRef()
        for clip_feature in clip_layer:
            clip_geom = clip_feature.GetGeometryRef()
            if in_geom.Intersects(clip_geom):
                out_feature = ogr.Feature(out_layer.GetLayerDefn())
                out_feature.SetGeometry(in_geom.Intersection(clip_geom))
                for i in range(0, in_layer_defn.GetFieldCount()):
                    out_feature.SetField(i, in_feature.GetField(i))
                out_layer.CreateFeature(out_feature)
                out_feature.Destroy()

    # 释放资源
    in_datasource.Destroy()
    clip_datasource.Destroy()
    out_datasource.Destroy()

def clip_shp_2(input_shp_path, clip_geometry, output_shp_path):
    ''' 根据clip_geometry(数组, 记录经纬度范围), 裁剪input_shp 并输出output_shp'''

    ogr.UseExceptions()

    # 打开输入数据源
    in_driver = ogr.GetDriverByName('ESRI Shapefile')
    in_datasource = in_driver.Open(input_shp_path, 0)
    if in_datasource is None:
        raise FileNotFoundError(f"无法打开输入文件: {input_shp_path}")
    in_layer = in_datasource.GetLayer()

    # 创建输出数据源
    out_driver = ogr.GetDriverByName('ESRI Shapefile')
    if out_driver is None:
        raise Exception('ESRI Shapefile 驱动不可用')
    if os.path.exists(output_shp_path):
        out_driver.DeleteDataSource(output_shp_path)
    out_datasource = out_driver.CreateDataSource(output_shp_path)
    out_layer = out_datasource.CreateLayer(
        in_layer.GetName(),
        srs=in_layer.GetSpatialRef(),
        geom_type=in_layer.GetGeomType()
    )

    # 复制输入图层的字段定义到输出图层
    in_layer_defn = in_layer.GetLayerDefn()
    for i in range(0, in_layer_defn.GetFieldCount()):
        field_defn = in_layer_defn.GetFieldDefn(i)
        out_layer.CreateField(field_defn)

    # 根据裁剪几何类型进行裁剪
    if isinstance(clip_geometry, ogr.Geometry):
        clip_geom = clip_geometry
    elif isinstance(clip_geometry, tuple) and len(clip_geometry) == 4:
        minx, miny, maxx, maxy = clip_geometry
        clip_geom = ogr.Geometry(ogr.wkbPolygon)
        ring = ogr.Geometry(ogr.wkbLinearRing)
        ring.AddPoint(minx, miny)
        ring.AddPoint(maxx, miny)
        ring.AddPoint(maxx, maxy)
        ring.AddPoint(minx, maxy)
        ring.AddPoint(minx, miny)
        clip_geom.AddGeometry(ring)

    # 进行裁剪操作
    for in_feature in in_layer:
        in_geom = in_feature.GetGeometryRef()
        if in_geom.Intersects(clip_geom):
            out_feature = ogr.Feature(out_layer.GetLayerDefn())
            out_feature.SetGeometry(in_geom.Intersection(clip_geom))
            for i in range(0, in_layer_defn.GetFieldCount()):
                out_feature.SetField(i, in_feature.GetField(i))
            out_layer.CreateFeature(out_feature)
            out_feature.Destroy()

    # 释放资源
    in_datasource.Destroy()
    out_datasource.Destroy()


if __name__ == '__main__':
    input_shp = r'c:\Users\lenovo\Desktop\heibei_osm_mainly_roads\heibei_osm_mainly_roads.shp'
    output_shp = r'c:\Users\lenovo\Desktop\heibei_osm_mainly_roads\beijing_osm_mainly_roads.shp'
    
    if False:
        # 使用shp文件进行裁剪
        clip_shp_path = 'clip.shp'
        clip_shp_1(input_shp, clip_shp_path, output_shp)
    else:
        # 使用经纬度范围进行裁剪
        latlon_bounds = (116.117, 39.704, 116.614, 40.186)
        clip_shp_2(input_shp, latlon_bounds, output_shp)






