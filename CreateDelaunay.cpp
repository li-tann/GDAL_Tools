#include <gdal.h>
#include <gdal_alg.h>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"

using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    argparse::ArgumentParser program("create_delaunay", "1.0");
    program.add_description("input points, construct delaunay network and output");

    program.add_argument("input")
        .help("points text file, with one line representing a point and a comma to separating 'x' and 'y', like: 'x,y'");

    program.add_argument("output")
        .help("output file, with one line representing a delaunay network, like:index:[i,j,k]; pos:[(x,y),(x,y),(x,y)])");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl<<std::endl;
        std::cerr << program;
        return 1;
    }

    /// log
    char* pgmptr = 0;
    _get_pgmptr(&pgmptr);
    fs::path exe_root(fs::path(pgmptr).parent_path());
    fs::path log_path = exe_root / "create_delaunay.log";
    auto my_logger = spdlog::basic_logger_mt("create_delaunay", log_path.string());


    string msg;


    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
		spdlog::info(msg);
        return rtn;
    };

    return_msg(0,"create_delaunay start.");

    string input_filepath = program.get<string>("input");
    string output_filepath = program.get<string>("output");

    ifstream ifs(input_filepath.c_str());
    if(!ifs.is_open()){
        return return_msg(-2,"'input' open failed.");
    }

    string str_line;
    vector<string> str_splited;
    vector<double> vec_x,vec_y;
    while(getline(ifs,str_line)){
        strSplit(str_line, str_splited, ",", true);
        if(str_splited.size() < 2)
            continue;
        vec_x.push_back(stod(str_splited[0]));
        vec_y.push_back(stod(str_splited[1]));
    }
    ifs.close();

    if(vec_x.size() < 3){
        return return_msg(-3,"number of valid data is less than 3.");
    }

    /// create delaunay

    double* x = new double[vec_x.size()];
    double* y = new double[vec_x.size()];

    for(int i=0; i<vec_x.size(); i++){
        x[i] = vec_x[i];
        y[i] = vec_y[i];
    }

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto dst = GDALTriangulationCreateDelaunay(vec_x.size(), x, y);

    if(dst->nFacets < 1){
        return return_msg(-3,"there is no available delaunay triangles.");
    }

    ofstream ofs(output_filepath.c_str());
    if(!ofs.is_open()){
        return return_msg(-4,"'output' open failed.");
    }

    for(int i=0; i<dst->nFacets; i++)
    {
        ofs<<"delaunay ["<<i<<"]:"<<endl;
        string index = "index";
        string pos = "pos";

        auto facet = dst->pasFacets[i];
        for(int j=0; j<3; j++){
            index += ", " + to_string(facet.anVertexIdx[j]);
            pos += ", (" + to_string(x[facet.anVertexIdx[j]]) + "," + to_string(y[facet.anVertexIdx[j]]) +")" ;
        }
        ofs << index <<endl;
        ofs << pos <<endl;
    }
    ofs.close();


    return return_msg(1, "create_delaunay end.");
}
