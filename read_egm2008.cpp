#include <iostream>
#include <fstream>
#include <chrono>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


#define EXE_NAME "_read_EGM2008"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

int main(int argc, char* argv[])
{

    auto start = chrono::system_clock::now();
    string msg;

    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));

    auto return_msg = [my_logger](int rtn, string msg){
        my_logger->info(msg);
        spdlog::info(msg);
        return rtn;
    };

    ///2种方式: 1.强度图, 2.相位图
    /// 强度图与相位图的共同参数 1.影像地址, 2.多视倍数, 3.打印方法, 4.最值设置( auto,2 / manual,1,0 ), 5.颜色表, 6.输出地址
    if(argc < 3){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [egm_filepath] [output_txtfilepath]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: egm_filepath.\n"
                " argv[2]: output_txtfilepath.\n";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    ifstream ifs(argv[1], ifstream::binary); //二进制读方式打开
    if (!ifs.is_open()) {
        printf("ERROR: File open failed...\nFilepath is %s",string(argv[1]));
        return false;
    }

    ofstream ofs(argv[2]);
    if(!ofs.is_open()){
         printf("ERROR: File write failed...\nFilepath is %s",string(argv[2]));
        return false;
    }

    long num = 0;
    float value;
    while (ifs.read((char*)&value, 4)) { //一直读到文件结束
        ofs<<value<<",";
        ++num;
    }
    ofs<<"\nnum:"<<num;

    ifs.close();
    ofs.close();

    return return_msg(1, EXE_NAME " end.");
}