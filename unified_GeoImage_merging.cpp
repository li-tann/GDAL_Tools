#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <filesystem>
#include <complex>
#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <gdal_priv.h>

#define EXE_NAME "_unified_geoimage_merging"

using namespace std;
namespace fs = std::filesystem;

string EXE_PLUS_FILENAME(string extention){
    return string(EXE_NAME)+"."+ extention;
}

/// TODO: 需要解决的问题
///         1. 由dem生成的简单shp能否与目标shp计算是否存在交集(应该可以, 参考LTIE的dem拼接代码(那里有算两个面矢量的相交比例))
///         2. 由目标shp是否能得到shp对应的经纬度范围
///         3. 拼接策略, a)是只拼接与目标shp有交集的DEM，但遇到不规则的shp时, 拼接结果的边边角角可能会有很多空值
///                     b)是将shp经纬度范围内的所有DEM都拼上，这样虽然不会有空值，但也失去了shp的形状特点


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

    if(argc < 4){
        msg =   EXE_PLUS_FILENAME("exe\n");
        msg +=  " manual: " EXE_NAME " [input] [method] [output]\n" 
                " argv[0]: " EXE_NAME ",\n"
                " argv[1]: input, dem_folder.\n"
                " argv[2]: input, target shp.\n"
                " argv[3]: input, merging method, 0:minimun, 1:maximum"
                " argv[4]: output, merged dem tif.";
        return return_msg(-1,msg);
    }

    return_msg(0,EXE_NAME " start.");

    /// 1.读取shp, 获取基本信息(经纬度范围, 生成矢量文件OGRGeometry )
    
    
    
    return return_msg(1, EXE_NAME " end.");
}
