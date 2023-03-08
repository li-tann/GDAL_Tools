#include <gdal_priv.h>

#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#define EXE_NAME "_statistics"

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
    CPLErr err;

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
                " argv[1]: input imgpath,\n"
                " argv[2]: band, such: \"1\" or \"1,2,3\".";
        return return_msg(-1,msg);
    }

    vector<int> bands;
    {
        string str_argv2 = argv[2];
        vector<string> vec_splited;
        strSplit(str_argv2,vec_splited,",");
        for(int i=0; i<vec_splited.size(); i++){
            bands.push_back(stoi(vec_splited[i]));
        }
    }
    
    return_msg(0,EXE_NAME " start.");

    GDALDataset* ds = static_cast<GDALDataset*>(GDALOpen(argv[1],GA_ReadOnly));
    if(ds == nullptr){
        return return_msg(-2,"ds is nullptr.");
    }

    for(int band : bands)
    {
        if(band < 1 || band > ds->GetRasterCount()){
            return_msg(-3,"band " + to_string(band) + "is out of ds.rasterCount, continue...\n");
            continue;
        }
        GDALRasterBand* rb = ds->GetRasterBand(band);
        
        double min, max, mean, stddev;
        err = rb->ComputeStatistics(FALSE, &min, &max, &mean, &stddev, NULL,NULL);
        if(err == CE_Failure){
            return_msg(-4,"band " + to_string(band) + "compute statistics failed, continue...\n");
            continue;
        }
        msg = "band " + to_string(band) + ":\n"
                "minium :" + to_string(min) + "\n" +
                "maxium :" + to_string(max) + "\n" +
                "mean   :" + to_string(mean) + "\n" + 
                "stdDev :" + to_string(stddev) + "\n";

        return_msg(0,msg);
    }

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