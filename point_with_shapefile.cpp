#include "vector_include.h"
#include <fmt/color.h>

/*
    sub_point_with_shp.add_argument("shapefile_path")
        .help("polygon shapefile.");
    
    sub_point_with_shp.add_argument("-p","--points")
        .help("point like: 'x,y' or 'lon,lat'.")
        .nargs(argparse::nargs_pattern::at_least_one);

    sub_point_with_shp.add_argument("-f","--file")
        .help("a file that records points, with each line representing a point like: 'x,y' or 'lon,lat'.");  

    sub_point_with_shp.add_argument("-s","--save")
        .help("save result as a file, default is print at terminal.");    
*/

int point_with_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_path = args->get<string>("shapefile_path");

    struct pos_xy{
        double x,y;
        bool within = false;
        pos_xy(double _x, double _y):x(_x),y(_y){}
    };

    vector<pos_xy> points;
    if(args->is_used("--point")){
        vector<string> str_points = args->get<vector<string>>("--points");
        vector<string> splited;
        for(const auto& str : str_points){
            strSplit(str, splited, ",", true);
            if(splited.size()>1){
                points.push_back(pos_xy(stod(splited[0]), stod(splited[1])));
            }
        }
    }

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
                    points.push_back(pos_xy(stod(splited[0]), stod(splited[1])));
                }
            }
        }
        else{
            PRINT_LOGGER(logger, warn, "--file is not a regular file.");
        }
    }

    if(points.size()<1){
        PRINT_LOGGER(logger, error, "the number of valid points is 0.");
        return -2;
    }

    GDALAllRegister();
    OGRRegisterAll();

    OGRPoint target_point;

    GDALDataset* dataset = (GDALDataset*)GDALOpenEx(shp_path.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
    if(!dataset){
        PRINT_LOGGER(logger, error, "dataset is nullptr.");
        return -3;
    }
    OGRLayer* layer = dataset->GetLayer(0);
    layer->ResetReading();

    OGRFeature* feature;
    // bool within = false;
    while ((feature = layer->GetNextFeature()) != NULL)
    {
        OGRGeometry* geometry = feature->GetGeometryRef();
        if (geometry != NULL)
        {
            for(auto& pos : points){
                target_point.setX(pos.x);
                target_point.setY(pos.y);
                if (geometry->Contains(&target_point)){
                    pos.within = true;
                }
            }
        }

        OGRFeature::DestroyFeature(feature);
    }

    GDALClose(dataset);

    std::string str_in = (fmt::format(fg(fmt::color::green),"in"));
    std::string str_out = (fmt::format(fg(fmt::color::red),"outside"));

    if(args->is_used("--save"))
    {
        string outpath = args->get<string>("--save");
        ofstream ofs(outpath);
        if(!ofs.is_open()){
            PRINT_LOGGER(logger, warn, fmt::format("write in [{}] failed, which will be printed on terminal.",outpath));
            for(auto& pos : points){
                PRINT_LOGGER(logger, info, "point("+to_string(pos.x)+","+to_string(pos.y)+") is " + (pos.within ? str_in : str_out) + " this shp file.");
            }
        }
        else{
            for(auto& pos : points)
            {
                ofs<<pos.x<<","<<pos.y<<","<<(pos.within ? "in" :"outside")<<std::endl;
            }
            ofs.close();
            PRINT_LOGGER(logger, info, fmt::format("relationship has been writed in file [{}] correctly.",outpath));
        }

    }
    else{
        for(auto& pos : points){
                PRINT_LOGGER(logger, info, "point("+to_string(pos.x)+","+to_string(pos.y)+") is " + (pos.within ? str_in : str_out) + " this shp file.");
            }
    }

    PRINT_LOGGER(logger, info, "point_with_shp success.");
    return 1;
}