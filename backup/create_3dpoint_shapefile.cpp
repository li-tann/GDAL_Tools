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

#define EXE_NAME "create_3dpoint_shapefile"

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
                " argv[1]: input, txt file, one line represents ont point, such as 'x,y,z', 'lon,lat,dn'.\n"
                " argv[2]: output, shp.\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    string txt_file = argv[1];
    string shp_file = argv[2];
    vector<xyz> points;
    
    ifstream ifs(argv[1]);
    if(!ifs.is_open()){
        return return_msg(-2, "ifs.open argv[1] failed.");
    }

    string str;
    vector<string> splited_strs;
    while(getline(ifs, str))
    {
        strSplit(str, splited_strs, ",");
        if(splited_strs.size()>2){
            xyz pos = xyz(std::stod(splited_strs[0]),std::stod(splited_strs[1]),std::stod(splited_strs[2]));
            points.push_back(pos);
        }
    }

    std::cout<<"points.size:"<<points.size()<<std::endl;
    if(points.size() < 1){
        return return_msg(-3, fmt::format("valid points.size({}) < 1",points.size()));
    }

    GDALDriver* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    if (shp_driver == nullptr) {
        return funcrst(false, "shp driver is nullptr.");
    }

    cout<<"default projection is WGS84\n";
    OGRSpatialReference spatialRef;
    spatialRef.SetWellKnownGeogCS("WGS84");

    GDALDataset* ds = shp_driver->Create(shp_file.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if (ds == nullptr) {
        return funcrst(false, "ds is nullptr.");
    }
    
    OGRLayer* layer = ds->CreateLayer("points", &spatialRef, wkbPoint, NULL);
    // OGRLayer* layer = ds->CreateLayer("points", nullptr, wkbPoint, NULL);
    if (layer == nullptr) {
        return funcrst(false, "layer is nullptr.");
    }

    /// layer中新建一个名为“id”的字段, 类型是int（在属性表中显示）
    OGRFieldDefn fieldDefn("ID", OFTInteger);
    fieldDefn.SetWidth(5);
    layer->CreateField(&fieldDefn);

    OGRFieldDefn fieldDefn_z("Z", OFTReal);
    // fieldDefn_z.SetWidth(32);
    fieldDefn_z.SetPrecision(3);
    layer->CreateField(&fieldDefn_z);


    OGRPoint point;
    
    int i=0;
    for (auto& p : points) {
        OGRFeature* poFeature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        std::cout<<fmt::format("[{}]:{},{},{}",i,p.x,p.y,p.z)<<std::endl;
        point.setX(p.x);
        point.setY(p.y);
        // point.setZ(p.z);
        poFeature->SetField("ID", i++);
        poFeature->SetField("Z", p.z);
        poFeature->SetGeometry(&point);
        if (layer->CreateFeature(poFeature) != OGRERR_NONE) {
            std::cout<<"create feature in shapefile failed."<<std::endl;
           return funcrst(false, "create feature in shapefile failed.");
        }
        OGRFeature::DestroyFeature(poFeature);
    }
    GDALClose(ds);


    return return_msg(1, EXE_NAME " end.");
}
