#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#define EXE_NAME "_determine_topological_relationship"
#define HISTOGRAM_SIZE 256

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

void strSplit(std::string input, std::vector<std::string>& output, std::string split, bool clearVector = true);

int main(int argc, char* argv[])
{
    GDALAllRegister();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: point(like: x,y),\n"
                " argv[2]: shp file.\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");


    vector<string> str_splited;
    double pos_x, pos_y;
    
    strSplit(argv[1], str_splited, ",", true);

    if(str_splited.size() < 2){
        return return_msg(-3,"number of valid data in argv[1] is less than 2.");
    }

    pos_x = stod(str_splited[0]);
    pos_y = stod(str_splited[1]);

    GDALAllRegister();
    OGRRegisterAll();

    GDALDataset* dataset = (GDALDataset*)GDALOpenEx(argv[2], GDAL_OF_VECTOR, NULL, NULL, NULL);
    if(!dataset){
        return return_msg(-4,"unvalid argv[2], open dataset failed.");
    }
    OGRLayer* layer = dataset->GetLayer(0);
    layer->ResetReading();

    OGRFeature* feature;
    bool within = false;
    while ((feature = layer->GetNextFeature()) != NULL)
    {
        OGRGeometry* geometry = feature->GetGeometryRef();
        if (geometry != NULL)
        {
            OGRwkbGeometryType type = geometry->getGeometryType();
            cout<<"geometry.type: "<<type<<endl;
            if(type != wkbPolygon && type != wkbMultiPolygon){
                return_msg(0, "unsupported geometry type.");
                continue;
            }
                
            // if (type == wkbPoint)
            // {
            //     OGRPoint* point = (OGRPoint*)geometry;
            //     double x = point->getX();
            //     double y = point->getY();
            //     printf("Point (%f %f)\n", x, y);
            // }
            // else if (type == wkbLineString)
            // {
            //     OGRLineString* lineString = (OGRLineString*)geometry;
            //     int count = lineString->getNumPoints();
            //     for (int i = 0; i < count; ++i)
            //     {
            //         OGRPoint point;
            //         lineString->getPoint(i, &point);
            //         double x = point.getX();
            //         double y = point.getY();
            //         printf("LineString Point %d (%f %f)\n", i + 1, x, y);
            //     }
            // }
            // else if (type == wkbPolygon)
            // {
            //     OGRPolygon* polygon = (OGRPolygon*)geometry;
            //     int count = polygon->getNumInteriorRings() + 1;
            //     for (int i = 0; i < count; ++i)
            //     {
            //         OGRLinearRing* ring;
            //         if (i == 0)
            //             ring = polygon->getExteriorRing();
            //         else
            //             ring = polygon->getInteriorRing(i - 1);
            //         int pointCount = ring->getNumPoints();
            //         for (int j = 0; j < pointCount; ++j)
            //         {
            //             OGRPoint point;
            //             ring->getPoint(j, &point);
            //             double x = point.getX();
            //             double y = point.getY();
            //             printf("Polygon Ring %d Point %d (%f %f)\n", i + 1, j + 1, x, y);
            //         }
            //     }
            // }

            /// 判断点与它的拓扑关系
            OGRPoint target_point;
            target_point.setX(pos_x);
            target_point.setY(pos_y);


            bool b = geometry->Intersect((OGRGeometry*)&target_point);
            within = within || b;
            if(within)
                break;
        }

        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(dataset);

    msg = "point("+to_string(pos_x)+","+to_string(pos_y)+") is " + (within ? "in" : "out") + " of this shp file.";
    return_msg(2, msg);

    return return_msg(1, EXE_NAME " end.");
}


void strSplit(std::string input, std::vector<std::string>& output, std::string split, bool clearVector)
{
    if(clearVector)
        output.clear();
    std::string::size_type pos1, pos2;
    pos1 = input.find_first_not_of(split);
    pos2 = input.find(split,pos1);

    if (pos1 == std::string::npos) {
        return;
    }
    if (pos2 == std::string::npos) {
        output.push_back(input.substr(pos1));
        return;
    }
    output.push_back(input.substr(pos1, pos2 - pos1));
    strSplit(input.substr(pos2 + 1), output, split,false);
    
}