#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <regex>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>
#include <ogrsf_frmts.h>

#include "datatype.h"

#define EXE_NAME "create_shapfile"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

/// 输入经纬度范围, 转换为OGRGeometry格式, 后面可计算交集
OGRGeometry* range_to_ogrgeometry(double lon_min, double lon_max, double lat_min, double lat_max);

int main(int argc, char* argv[])
{
    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [point.txt] [output.shp]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, txt file, one line represents ont point, such as 'x,y', 'lon,lat'.\n"
                " argv[2]: output, shp.\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    string txt_file = argv[1];
    string shp_file = argv[2];
    vector<xy> points;
    
    ifstream ifs(argv[1]);
    if(!ifs.is_open()){
        return return_msg(-2, "ifs.open argv[1] failed.");
    }

    string str;
    vector<string> splited_strs;
    while(getline(ifs, str))
    {
        strSplit(str, splited_strs, ",");
        if(splited_strs.size()>1){
            xy pos = xy(std::stod(splited_strs[0]),std::stod(splited_strs[1]));
            points.push_back(pos);
        }
    }


    if(points.size() < 3){
        return return_msg(-3, fmt::format("valid points.size({}) < 3",points.size()));
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

    cout<<"default projection is WGS84\n";
    OGRSpatialReference spatialRef;
    spatialRef.SetWellKnownGeogCS("WGS84");
    OGRLayer* layer = ds->CreateLayer("layer", &spatialRef, wkbPolygon, NULL);
    if (layer == nullptr) {
        return funcrst(false, "layer is nullptr.");
    }

    OGRFeatureDefn* featureDefn = layer->GetLayerDefn();
    OGRFeature* feature = OGRFeature::CreateFeature(featureDefn);
    OGRErr err = feature->SetGeometry((OGRGeometry*)polygen);
    if (err != OGRERR_NONE) {
        if (err == OGRERR_UNSUPPORTED_GEOMETRY_TYPE) {
            return funcrst(false, "unsupported geometry type.");
        }
        else{
            return funcrst(false, "unknown setGeometry error.");
        }
    }


    if (layer->CreateFeature(feature) != OGRERR_NONE) {
        return funcrst(false, "create feature in shapefile failed.");
    }


    OGRFeature::DestroyFeature(feature);
    GDALClose(ds);



    
    return return_msg(1, EXE_NAME " end.");
}
