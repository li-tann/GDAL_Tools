from osgeo import ogr


def shp_add_defn(shp_path, defn_key:str, defn_value:int):
    '''
    int key
    '''
    ogr.UseExceptions()

    driver = ogr.GetDriverByName('ESRI Shapefile')
    ds:ogr.DataSource  = driver.Open(shp_path, 1)
    layer:ogr.Layer = ds.GetLayer()

    field_defn = ogr.FieldDefn(defn_key, ogr.OFTInteger)
    layer.CreateField(field_defn)

    for feature in layer:
        feature.SetField(defn_key, defn_value)
        layer.SetFeature(feature)

    ds.Destroy()


# def merge_shps(input_shp_paths, output_shp_path, desired_field_names):

#     ogr.UseExceptions()

#     # 获取ESRI Shapefile驱动
#     driver = ogr.GetDriverByName('ESRI Shapefile')

#     # 创建输出数据源
#     output_ds = driver.CreateDataSource(output_shp_path)

#     # 假设第一个SHP文件的空间参考和几何类型作为输出的标准
#     first_ds:ogr.DataSource = driver.Open(input_shp_paths[0], 0)
#     if first_ds is None:
#         raise ValueError(f"Cann't open shapefile: {input_shp_paths[0]}")
#     first_layer:ogr.Layer = first_ds.GetLayer()
#     srs = first_layer.GetSpatialRef()
#     geom_type = first_layer.GetGeomType()

#     # 创建输出图层
#     output_layer = output_ds.CreateLayer('merged', srs, geom_type)

#     # 获取并添加所需的字段定义
#     for field_name in desired_field_names:
#         field_defn = first_layer.GetLayerDefn().GetFieldDefnByName(field_name)
#         tmp:ogr.FeatureDefn = first_layer.GetLayerDefn()
#         first_layer.getfie
#         output_layer.CreateField(field_defn)

#     # 遍历每个输入SHP文件
#     for shp_path in input_shp_paths:
#         ds = driver.Open(shp_path, 0)
#         if ds is None:
#             print(f"Cann't open shapefile: {shp_path}, skip it.")
#             continue
#         layer = ds.GetLayer()

#         for feature in layer:
#             new_feature = ogr.Feature(output_layer.GetLayerDefn())
#             for field_name in desired_field_names:
#                 value = feature.GetField(field_name)
#                 new_feature.SetField(field_name, value)
#             geom = feature.GetGeometryRef()
#             new_feature.SetGeometry(geom)
#             output_layer.CreateFeature(new_feature)

#         ds.Destroy()

#     output_ds.Destroy()
#     first_ds.Destroy()

def merge_shps(input_shp_paths, output_shp_path, desired_field_names):

    ogr.UseExceptions()
    
    # 获取ESRI Shapefile驱动
    driver = ogr.GetDriverByName('ESRI Shapefile')

    # 创建输出数据源
    output_ds = driver.CreateDataSource(output_shp_path)

    # 假设第一个SHP文件的空间参考和几何类型作为输出的标准
    first_ds = driver.Open(input_shp_paths[0], 0)
    if first_ds is None:
        raise ValueError(f"无法打开SHP文件: {input_shp_paths[0]}")
    first_layer = first_ds.GetLayer()
    srs = first_layer.GetSpatialRef()
    geom_type = first_layer.GetGeomType()

    # 创建输出图层
    output_layer = output_ds.CreateLayer('merged', srs, geom_type)

    # 获取并添加所需的字段定义
    first_layer_defn = first_layer.GetLayerDefn()
    for field_name in desired_field_names:
        field_index = first_layer_defn.GetFieldIndex(field_name)
        if field_index >= 0:
            field_defn = first_layer_defn.GetFieldDefn(field_index)
            output_layer.CreateField(field_defn)
        else:
            print(f"字段 {field_name} 不存在于第一个SHP文件中，跳过此字段")

    # 遍历每个输入SHP文件
    for shp_path in input_shp_paths:
        ds = driver.Open(shp_path, 0)
        if ds is None:
            print(f"无法打开SHP文件: {shp_path}，跳过此文件")
            continue
        layer = ds.GetLayer()

        for feature in layer:
            new_feature = ogr.Feature(output_layer.GetLayerDefn())
            for field_name in desired_field_names:
                field_index = layer.GetLayerDefn().GetFieldIndex(field_name)
                if field_index >= 0:
                    value = feature.GetField(field_index)
                    new_feature.SetField(field_name, value)
            geom = feature.GetGeometryRef()
            new_feature.SetGeometry(geom)
            output_layer.CreateFeature(new_feature)

        ds.Destroy()

    output_ds.Destroy()
    first_ds.Destroy()


if __name__ == '__main__':
    
    if False:
        shp_add_defn(r'c:\Users\lenovo\Desktop\test\iw3_burst_1.shp', 'count', 1)
        shp_add_defn(r'c:\Users\lenovo\Desktop\test\iw3_burst_2.shp', 'count', 2)
        shp_add_defn(r'c:\Users\lenovo\Desktop\test\iw3_burst_3.shp', 'count', 3)


    input_shp_paths = [
        r'c:\Users\lenovo\Desktop\test\iw3_burst_1.shp',
        r'c:\Users\lenovo\Desktop\test\iw3_burst_2.shp',
        r'c:\Users\lenovo\Desktop\test\iw3_burst_3.shp'
        ]
    output_shp_path =r'c:\Users\lenovo\Desktop\test\iw3_burst_123_merged.shp'
    desired_field_names = ['count']
    merge_shps(input_shp_paths, output_shp_path, desired_field_names)