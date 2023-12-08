#include <gdal.h>
#include <gdal_alg.h>
#include <iostream>
#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#define EXE_NAME "create_Delaunay"
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

    if(argc < 2){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: points txt(like: x,y\\n....),\n"
                " argv[2]: delaunay infomation txt(such as, index:[i,j,k]; pos:[(x,y),(x,y),(x,y)]),\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    ifstream ifs(argv[1]);
    if(!ifs.is_open()){
        return return_msg(-2,"argv[1] open failed.");
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
    auto dst = GDALTriangulationCreateDelaunay(vec_x.size(),x,y);

    if(dst->nFacets < 1){
        return return_msg(-3,"there is no available delaunay triangles.");
    }

    ofstream ofs(argv[2]);
    if(!ofs.is_open()){
        return return_msg(-4,"argv[2] open failed.");
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