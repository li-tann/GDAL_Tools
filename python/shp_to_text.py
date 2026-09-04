from osgeo import ogr, osr
import argparse

ogr.UseExceptions()

def shp_to_postgis_geom(shp_path):
    driver = ogr.GetDriverByName('ESRI Shapefile')
    data_source = driver.Open(shp_path, 0)
    if data_source is None:
        raise FileNotFoundError(f'Could not open {shp_path}')
    layer = data_source.GetLayer()
    srs = layer.GetSpatialRef()
    if srs is None:
        raise ValueError(f'No spatial reference found in {shp_path}')
    epsg_code = srs.GetAuthorityCode(None)
    if epsg_code is None:
        raise ValueError(f'Could not determine EPSG code for {shp_path}')
    geom_strs = []
    for feature in layer:
        geom = feature.GetGeometryRef()
        if geom.GetGeometryName() == 'POLYGON':
            ring = geom.GetGeometryRef(0)
            num_points = ring.GetPointCount()
            coord_strs = []
            for i in range(num_points):
                x, y, _ = ring.GetPoint(i)
                coord_str = f"{x} {y}"
                coord_strs.append(coord_str)
            coord_sequence = ', '.join(coord_strs)
            polygon_str = f"POLYGON(({coord_sequence}))"
            postgis_geom_str = f"ST_GeomFromText('{polygon_str}', {epsg_code})"
            geom_strs.append(postgis_geom_str)
    data_source.Destroy()
    return geom_strs

def main():
    parser = argparse.ArgumentParser(description='Convert SHP file to PostGIS geometry text.')
    parser.add_argument('shp_path', help='Path to the SHP file.')
    parser.add_argument('txt_path', help='Path to print txt file.')
    args = parser.parse_args()
    try:
        results = shp_to_postgis_geom(args.shp_path)
        for result in results:
            print(result)

        with open(args.txt_path, 'w', encoding='utf-8') as f:
            for result in results:
                f.write(result)
    except (FileNotFoundError, ValueError) as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()