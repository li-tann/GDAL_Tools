#include "vector_include.h"

/*
    sub_create_point_shp.add_argument("output_shapefile")
        .help("output shapefile filepath");

    sub_create_point_shp.add_argument("-p", "--points")
        .help("point like: 'lon,lat'.")
        .nargs(argparse::nargs_pattern::at_least_one);

    sub_create_point_shp.add_argument("-f", "--file")
        .help("a file that records points, with each line representing a point like: 'lon,lat'.");
*/


int create_2dpoint_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
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

    if(points.size() < 1){
        PRINT_LOGGER(logger, error, "points.size < 1");
        return -2;
    }

    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDriver* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (shp_driver == nullptr) {
        PRINT_LOGGER(logger, error, "shp_driver is nullptr.");
        return -3;
    }

    PRINT_LOGGER(logger, info, "default projection is WGS84");
    OGRSpatialReference spatialRef;
    spatialRef.SetWellKnownGeogCS("WGS84");

    GDALDataset* ds = shp_driver->Create(shp_file.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if (ds == nullptr) {
        PRINT_LOGGER(logger, error, "ds is nullptr.");
        return -3;
    }
    
    OGRLayer* layer = ds->CreateLayer("points", &spatialRef, wkbPoint, NULL);
    if (layer == nullptr) {
        PRINT_LOGGER(logger, error, "layer is nullptr.");
        return -3;
    }

    /// layer中新建一个名为“id”的字段, 类型是int（在属性表中显示）
    OGRFieldDefn * fieldDefn = new OGRFieldDefn("ID", OFTInteger);
    fieldDefn->SetWidth(5);
    layer->CreateField(fieldDefn);

    OGRPoint point;
    
    int i=0;
    for (auto& p : points) {
        OGRFeature* poFeature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        // std::cout<<fmt::format("[{}]:{},{}",i,p.x,p.y)<<std::endl;
        point.setX(p.x);
        point.setY(p.y);
        poFeature->SetField("ID", i++);
        poFeature->SetGeometry(&point);
        if (layer->CreateFeature(poFeature) != OGRERR_NONE) {
            PRINT_LOGGER(logger, error, "create feature in shapefile failed.");
            return -4;    
        }
        OGRFeature::DestroyFeature(poFeature);
    }
    GDALClose(ds);

    PRINT_LOGGER(logger, info, "create_2dpoint_shp success.");
    return 1;
}

/*
    sub_create_3dpoint_shp.add_argument("output_shapefile")
        .help("output shapefile filepath");

    sub_create_3dpoint_shp.add_argument("-p", "--points")
        .help("point like: 'lon,lat,val'.")
        .nargs(argparse::nargs_pattern::at_least_one);

    sub_create_3dpoint_shp.add_argument("-f", "--file")
        .help("a file that records points, with each line representing a point like: 'lon,lat,val'.");
*/

int create_3dpoint_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_file = args->get<string>("output_shapefile");
    vector<xyz> points;

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
                    if(splited.size() < 3)
                        continue;
                    points.push_back(xyz(stod(splited[0]), stod(splited[1]), stod(splited[2])));
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
            if(splited.size()>2){
                points.push_back(xyz(stod(splited[0]), stod(splited[1]), stod(splited[2])));
            }
        }
    }
    else{
        PRINT_LOGGER(logger, error, "both --file and --points are not used.");
        return -1;
    }

    if(points.size() < 1){
        PRINT_LOGGER(logger, error, "points.size < 1");
        return -2;
    }

    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDriver* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (shp_driver == nullptr) {
        PRINT_LOGGER(logger, error, "shp_driver is nullptr.");
        return -3;
    }

    PRINT_LOGGER(logger, info, "default projection is WGS84");
    OGRSpatialReference spatialRef;
    spatialRef.SetWellKnownGeogCS("WGS84");

    GDALDataset* ds = shp_driver->Create(shp_file.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if (ds == nullptr) {
        PRINT_LOGGER(logger, error, "ds is nullptr.");
        return -3;
    }
    
    OGRLayer* layer = ds->CreateLayer("points", &spatialRef, wkbPoint, NULL);
    if (layer == nullptr) {
        PRINT_LOGGER(logger, error, "layer is nullptr.");
        return -3;
    }

    /// layer中新建一个名为“id”的字段, 类型是int（在属性表中显示）
    OGRFieldDefn * fieldDefn = new OGRFieldDefn("ID", OFTInteger);
    fieldDefn->SetWidth(5);
    layer->CreateField(fieldDefn);

    OGRFieldDefn fieldDefn_val("VAL", OFTReal);
    fieldDefn_val.SetPrecision(3);
    layer->CreateField(&fieldDefn_val);

    OGRPoint point;
    
    int i=0;
    for (auto& p : points) {
        OGRFeature* poFeature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        // std::cout<<fmt::format("[{}]:{},{}",i,p.x,p.y)<<std::endl;
        point.setX(p.x);
        point.setY(p.y);
        poFeature->SetField("ID", i++);
        poFeature->SetField("VAL", p.z);
        poFeature->SetGeometry(&point);
        if (layer->CreateFeature(poFeature) != OGRERR_NONE) {
            PRINT_LOGGER(logger, error, "create feature in shapefile failed.");
            return -4;    
        }
        OGRFeature::DestroyFeature(poFeature);
    }
    GDALClose(ds);

    PRINT_LOGGER(logger, info, "create_2dpoint_shp success.");
    return 1;
}