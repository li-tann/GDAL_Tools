#include "vector_include.h"

/*
    sub_create_polygon_shp.add_argument("output_shapefile")
        .help("output shapefile filepath");

    sub_create_polygon_shp.add_argument("-p", "--points")
        .help("point like: 'lon,lat'.")
        .nargs(argparse::nargs_pattern::at_least_one);

    sub_create_polygon_shp.add_argument("-f", "--file")
        .help("a file that records points, with each line representing a point like: 'lon,lat'.");
*/

int create_polygon_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_file = args->get<string>("output_shapefile");
    vector<xy> points;

    if(args->is_used("--file")){
        string filepath = args->get<string>("--file");
        fs::path point_path(filepath.c_str());
        if(fs::is_regular_file(point_path))
        {
            ifstream ifs(point_path.string());
            if(!ifs.is_open()){
                PRINT_LOGGER(logger, warn, "--file, ifs.open failed.");
            }
            else{
                string str;
                vector<string> splited;
                while(getline(ifs,str))
                {
                    strSplit(str, splited, ",", true);
                    if(splited.size() < 2)
                        continue;
                    points.push_back(xy(stod(splited[0]), stod(splited[1])));
                }
            }
        }
        else{
            PRINT_LOGGER(logger, warn, "--file is not a regular file.");
        }
    }
    else if(args->is_used("--points")){
        vector<string> str_points = args->get<vector<string>>("--points");
        vector<string> splited;
        for(const auto& str : str_points){
            strSplit(str, splited, ",", true);
            if(splited.size()>1){
                points.push_back(xy(stod(splited[0]), stod(splited[1])));
            }
        }
    }
    else{
        PRINT_LOGGER(logger, error, "both --file and --points are not used.");
        return -1;
    }

    if(points.size() < 3){
        PRINT_LOGGER(logger, error, "points.size < 3");
        return -2;
    }



    OGRPolygon* polygen = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
    OGRLinearRing* ring = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
    OGRPoint point;

    for(auto& iter : points)
    {
        point.setX(iter.x); point.setY(iter.y);
        ring->addPoint(&point);
    }
    point.setX(points[0].x); point.setY(points[0].y);
    ring->addPoint(&point);
    ring->closeRings();
    polygen->addRing(ring);


    GDALDriver* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (shp_driver == nullptr) {
        return funcrst(false, "shp driver is nullptr.");
    }

    GDALDataset* ds = shp_driver->Create(shp_file.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if (ds == nullptr) {
        return funcrst(false, "ds is nullptr.");
    }

    PRINT_LOGGER(logger, info, "default projection is WGS84");
    OGRSpatialReference spatialRef;
    spatialRef.SetWellKnownGeogCS("WGS84");
    OGRLayer* layer = ds->CreateLayer("layer", &spatialRef, wkbPolygon, NULL);
    if (layer == nullptr) {
        return funcrst(false, "layer is nullptr.");
    }

    /// layer中新建一个名为“name”的字段, 类型是string（在属性表中显示）
    OGRFieldDefn * fieldDefn = new OGRFieldDefn("name", OFTString);
    layer->CreateField(fieldDefn);

    OGRFeatureDefn* featureDefn = layer->GetLayerDefn();
    OGRFeature* feature = OGRFeature::CreateFeature(featureDefn);
    OGRErr err = feature->SetGeometry((OGRGeometry*)polygen);
    feature->SetField("name","temp");
    
    if (err != OGRERR_NONE) {
        if (err == OGRERR_UNSUPPORTED_GEOMETRY_TYPE) {
            PRINT_LOGGER(logger, error, "unsupported geometry type.");
            return -4;
        }
        else{
            PRINT_LOGGER(logger, error, "unknown setGeometry error.");
            return -5;
        }
    }


    if (layer->CreateFeature(feature) != OGRERR_NONE) {
        PRINT_LOGGER(logger, error, "create feature in shapefile failed.");
        return -6;
    }


    OGRFeature::DestroyFeature(feature);
    GDALClose(ds);


    PRINT_LOGGER(logger, info, "create_polygon_shp success.");
    return 1;
}