#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <iostream>
#include <fstream>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "datatype.h"

#define EXE_NAME "merge_sar_image"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

enum class merge_Method_{percentage};

int main(int argc, char* argv[])
{
    MAIN_INIT(EXE_NAME);

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual:\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input_filepath_up, only supported 4bands, 8bit png image.\n"
                " argv[2]: input_filepath_down, only supported 4bands, 8bit png image.\n"
                " argv[3]: merge method, such as: 'percentage,0.3,0.7', ''.\n"
                " argv[4]: print filepath (*.png).\n";
        return return_msg(-1,msg);
    }
    return return_msg(1, EXE_NAME " start.");

    GDALAllRegister();

    

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

bool print_to_power();

bool print_to_phase();