#include "vector_include.h"
#include <fmt/color.h>

/*
    sub_point_with_shp.add_argument("shapefile_path")
        .help("polygon shapefile.");
    
    sub_point_with_shp.add_argument("-p","--polygen")
        .help("polygen like: 'lon_0,lat_0;lon_1,lat_1;lon_2,lat_2;lon_3,lat_3;lon_0,lat_0'.")
        .nargs(argparse::nargs_pattern::at_least_one);

    sub_point_with_shp.add_argument("-f","--file")
        .help("a file that records points, with each line representing a point like: ''lon_0,lat_0;lon_1,lat_1;lon_2,lat_2;lon_0,lat_0'.");  

    sub_point_with_shp.add_argument("-s","--save")
        .help("save result as a file, default is print at terminal.");    
*/

int polygen_with_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_path = args->get<string>("shapefile_path");

    struct pos_xy{
        double x,y;
        pos_xy(double _x, double _y):x(_x),y(_y){}
        std::string to_str(){
            return fmt::format("{},{}",x,y);
        }
    };

    struct polygen_pos : vector<pos_xy>{
        bool within = false;
        std::string to_str(){
            std::string dst = "";
            for(int i=0; i<size(); i++){
                dst += data()[i].to_str() + (i < size()-1 ? ";" : "");
            }
            return dst;
        }
    };

    vector<polygen_pos> polygens;
    if(args->is_used("--polygen"))
    {
        vector<string> str_points = args->get<vector<string>>("--polygen");
        vector<string> splited;
        for(const auto& str : str_points)
        {
            strSplit(str, splited, ";", true);
            if(splited.size() < 4)
                continue;
            polygen_pos tmp_polygen;
            vector<string> tmp_splited;
            for(auto& str2 : splited)
            {
                strSplit(str2, tmp_splited, ",", true);
                if(tmp_splited.size() > 1)
                    tmp_polygen.push_back(pos_xy(stod(tmp_splited[0]), stod(tmp_splited[1])));
            }
            if(tmp_polygen.size() < 4)
                continue;
            polygens.push_back(tmp_polygen);
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
                    strSplit(str, splited, ";", true);
                    if(splited.size() < 4)
                        continue;
                    polygen_pos tmp_polygen;
                    vector<string> tmp_splited;
                    for(auto& str2 : splited)
                    {
                        strSplit(str2, tmp_splited, ",", true);
                        if(tmp_splited.size() > 1)
                            tmp_polygen.push_back(pos_xy(stod(tmp_splited[0]), stod(tmp_splited[1])));
                    }
                    if(tmp_polygen.size() < 4)
                        continue;
                    polygens.push_back(tmp_polygen);
                }
            }
        }
        else{
            PRINT_LOGGER(logger, warn, "--file is not a regular file.");
        }
    }

    std::cout<<fmt::format("polygen.size: {}", polygens.size())<<std::endl;
    if(polygens.size()<1){
        PRINT_LOGGER(logger, error, "the number of valid polygens is 0.");
        return -2;
    }

    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
    
    OGRPoint point;

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
            for(auto& poly : polygens)
            {
                OGRLinearRing ring;
                OGRPolygon target_polygen;
                for(auto& pnt : poly)
                {
                    point.setX(pnt.x); point.setY(pnt.y);
                    ring.addPoint(&point);
                }
                ring.closeRings();
                target_polygen.addRing(&ring);
                if (geometry->Intersect(&target_polygen)){
                    poly.within = true;
                }
                std::cout<<fmt::format("poly.within? {}",(poly.within ? "true" : "false"))<<std::endl;
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
            for(auto& pos : polygens){
                PRINT_LOGGER(logger, info, fmt::format("polygen({}) is {} this shp file.",pos.to_str(),(pos.within ? str_in : str_out)));
            }
        }
        else{
            for(auto& pos : polygens)
            {
                // ofs<<pos.x<<","<<pos.y<<","<<(pos.within ? "in" :"outside")<<std::endl;
                ofs<<fmt::format("{:<4} - polygen({}) is {} this shp file.",(pos.within ? str_in : str_out),pos.to_str());
            }
            ofs.close();
            PRINT_LOGGER(logger, info, fmt::format("relationship has been writed in file [{}] correctly.",outpath));
        }

    }
    else{
        for(auto& pos : polygens){
            PRINT_LOGGER(logger, info, fmt::format("polygen({}) is {} this shp file.",pos.to_str(),(pos.within ? str_in : str_out)));
        }
    }

    PRINT_LOGGER(logger, info, "polygen_with_shp success.");
    return 1;
}


int polygen_overlap_rate(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp2 = args->get<string>("shp1");
    string shp1 = args->get<string>("shp2");

    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
    
    OGRPoint point;

    OGRGeometry* geometry1 = nullptr;
    {
        GDALDataset* dataset = (GDALDataset*)GDALOpenEx(shp1.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
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
            OGRGeometry* tmp_geometry1 = feature->GetGeometryRef();
            if(tmp_geometry1 != NULL){
                std::cout<<"area1:"<<tmp_geometry1->toPolygon()->get_Area()<<std::endl;
                geometry1 = tmp_geometry1->clone();
                break;
            }
            // OGRFeature::DestroyFeature(feature);
        }
        // GDALClose(dataset);
    }
    if(!geometry1){
        PRINT_LOGGER(logger, error, "geometry1 is nullptr.");
        return -3;
    }

    OGRGeometry* geometry2 = nullptr;
    {
        GDALDataset* dataset2 = (GDALDataset*)GDALOpenEx(shp2.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
        if(!dataset2){
            PRINT_LOGGER(logger, error, "dataset2 is nullptr.");
            return -3;
        }
        OGRLayer* layer2= dataset2->GetLayer(0);
        layer2->ResetReading();

        OGRFeature* feature2;
        // bool within = false;
        
        while ((feature2 = layer2->GetNextFeature()) != NULL)
        {
            OGRGeometry* tmp_geometry2 = feature2->GetGeometryRef();
            if(tmp_geometry2 != NULL){
                std::cout<<"area2:"<<tmp_geometry2->toPolygon()->get_Area()<<std::endl;
                geometry2 = tmp_geometry2->clone();
                break;
            }
            // OGRFeature::DestroyFeature(feature);
        }
        // GDALClose(dataset);
    }
    if(!geometry2){
        PRINT_LOGGER(logger, error, "geometry2 is nullptr.");
        return -3;
    }

    auto intersection = geometry1->Intersection(geometry2);
    if(intersection->IsEmpty()){
        PRINT_LOGGER(logger, error, "intersection is empty.");
        return -3;
    }

    double area1 = geometry1->toPolygon()->get_Area();
    double area2 = geometry1->toPolygon()->get_Area();
    double area_i = intersection->toPolygon()->get_Area();
    PRINT_LOGGER(logger, info, fmt::format("geometry1. area: {}.", area1));
    PRINT_LOGGER(logger, info, fmt::format("geometry2. area: {}.", area2));
    PRINT_LOGGER(logger, info, fmt::format("intersection. area: {}.", area_i));

    PRINT_LOGGER(logger, info, fmt::format("overlap.rate: {}.", area_i / MIN(area1, area2)));


    PRINT_LOGGER(logger, info, "overlay rate finished.");
    return 1;
}