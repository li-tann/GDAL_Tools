#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <fstream>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#define EXE_NAME "_determine_topological_relationship"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

void strSplit(std::string input, std::vector<std::string>& output, std::string split, bool clearVector = true);
double spend_time(decltype (std::chrono::system_clock::now()) start);

int main(int argc, char* argv[])
{

    auto start = chrono::system_clock::now();

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

    OGRPoint target_point;
    target_point.setX(pos_x);
    target_point.setY(pos_y);

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
            if(0)//old
            {
                OGRwkbGeometryType type = geometry->getGeometryType();
                cout<<"geometry.type: "<<type<<endl;
                if(type != wkbPolygon && type != wkbMultiPolygon){
                    return_msg(0, "unsupported geometry type.");
                    continue;
                }

                /// 判断点与它的拓扑关系
                bool b = geometry->Intersect((OGRGeometry*)&target_point);
                within = within || b;
                if(within)
                    break;
            }
            if (geometry->Contains(&target_point))
            {
                within = true;
            }
        }

        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(dataset);

    cout<<"spend time:"<<spend_time(start)<<"s"<<endl;
#ifdef WIN32
    std::string str_in = "\x1b[1;32mi\x1b[1;32mn\x1b[0m ";
    std::string str_out = "\x1b[1;31mo\x1b[1;31mu\x1b[1;31mt\x1b[1;31ms\x1b[1;31mi\x1b[1;31md\x1b[1;31me\x1b[0m ";
#elif define(__linux__)
    std::string str_in = "\033[32mi\033[32mn\033[0m ";
    std::string str_out = "\033[31mo\033[31mu\033[31mt\033[31ms\033[31mi\033[31md\033[31me\033[0m ";
#endif

    msg = "point("+to_string(pos_x)+","+to_string(pos_y)+") is " + (within ? str_in : str_out) + "this shp file.";
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

double spend_time(decltype (std::chrono::system_clock::now()) start)
{
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double second = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    return second;
}