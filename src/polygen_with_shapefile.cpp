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
            std::string dst;
            for(int i=0; i<size(); i++){
                dst += this->at(i).to_str() + (i < size()-1 ? ";" : "\n");
            }
        }
    };

    vector<polygen_pos> polygens;
    if(args->is_used("--point"))
    {
        vector<string> str_points = args->get<vector<string>>("--points");
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

    if(polygens.size()<1){
        PRINT_LOGGER(logger, error, "the number of valid polygens is 0.");
        return -2;
    }

    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
    
    OGRPoint point;
    OGRLinearRing ring;
    OGRPolygon target_polygen;

    // OGRPolygon* polygen = (OGRPolygon*)OGRGeometryFactory::createGeometry(wkbPolygon);
    // OGRLinearRing* ring = (OGRLinearRing*)OGRGeometryFactory::createGeometry(wkbLinearRing);
    // OGRPoint point;

    // for(auto& iter : points)
    // {
    //     point.setX(iter.x); point.setY(iter.y);
    //     ring->addPoint(&point);
    // }
    // point.setX(points[0].x); point.setY(points[0].y);
    // ring->addPoint(&point);
    // ring->closeRings();
    // polygen->addRing(ring);

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
                for(auto& pnt : poly)
                {
                    point.setX(pnt.x); point.setY(pnt.y);
                    ring.addPoint(&point);
                }
                ring.closeRings();
                target_polygen.addRing(&ring);

                if (geometry->Contains(&target_polygen)){
                    poly.within = true;
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