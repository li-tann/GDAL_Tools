#include "raster_include.h"
#include <nlohmann/json.hpp>

struct quadtree{

    quadtree(int sx, int sy, int w, int h, int d = 1)
        :start_x(sx), start_y(sy), width(w), height(h), depth(d){
    }
    int start_x, start_y;
    int width, height;

    int depth = 0;
    quadtree* qd_topleft = nullptr;
    quadtree* qd_topright = nullptr;
    quadtree* qd_downleft = nullptr;
    quadtree* qd_downright = nullptr;

    bool new_quad(int max_depth){
        if(width < 2 || height < 2 || depth >= max_depth){
            return false;
        }
        
        int cen_x = floor(width/2.0)+start_x;
        int cen_y = floor(height/2.0)+start_y;

        // std::cout<<fmt::format("topleft:{},{}, size:{}x{}, depth:{}",start_x, start_y, height, width,depth)<<std::endl;

        qd_topleft = new quadtree(start_x, start_y, width/2, height/2, depth+1);
        qd_topright = new quadtree(cen_x, start_y, width - width/2, height/2, depth+1);
        qd_downleft = new quadtree(start_x, cen_y, width/2, height - height/2, depth+1);
        qd_downright = new quadtree(cen_x, cen_y, width - width/2, height - height/2, depth+1);

        return true;
    }

    void iter(int max_depth, double thres, char** mask, std::function<bool(char**,int, int, int, int, double)> func){
        // std::cout<<fmt::format("iter_before_func: depth:{}; topleft:{},{}; size:{},{}; \n", depth, start_y, start_x, height, width);
        if(func(mask, start_x, start_y, width, height, thres)){
            // std::cout<<fmt::format("iter_after_func: depth:{}; topleft:{},{}; size:{},{}; \n", depth, start_y, start_x, height, width);
            if(new_quad(max_depth)){
                // std::cout<<fmt::format("iter_after_new_quad: depth:{}; topleft:{},{}; size:{},{}; \n", depth, start_y, start_x, height, width);
                qd_topleft->iter(max_depth, thres, mask, func);
                qd_topright->iter(max_depth, thres, mask, func);
                qd_downleft->iter(max_depth, thres, mask, func);
                qd_downright->iter(max_depth, thres, mask, func);
            }
        }
    }

    void close(){
        if(qd_topleft){
            qd_topleft->close();
            qd_topleft = nullptr;
        }
        if(qd_topright){
            qd_topright->close();
            qd_topright = nullptr;
        }
        if(qd_downleft){
            qd_downleft->close();
            qd_downleft = nullptr;
        }
        if(qd_downright){
            qd_downright->close();
            qd_downright = nullptr;
        }
    }

    int max_depth(){
        int max_dep = depth;
        if(qd_topleft){
            int corner_max_dep = qd_topleft->max_depth();
            max_dep = max_dep > corner_max_dep ? max_dep : corner_max_dep;
        }
        if(qd_topright){
            int corner_max_dep = qd_topright->max_depth();
            max_dep = max_dep > corner_max_dep ? max_dep : corner_max_dep;
        }
        if(qd_downleft){
            int corner_max_dep = qd_downleft->max_depth();
            max_dep = max_dep > corner_max_dep ? max_dep : corner_max_dep;
        }
        if(qd_downright){
            int corner_max_dep = qd_downright->max_depth();
            max_dep = max_dep > corner_max_dep ? max_dep : corner_max_dep;
        }
        return depth;
    }

    int tree_count()
    {
        if(!qd_topleft) /// 有一个指针为空, 说明四个都为空
            return 1;

        return qd_topleft->tree_count() + qd_topright->tree_count() + qd_downleft->tree_count() + qd_downright->tree_count();
    }

    nlohmann::ordered_json to_json()
    {
        nlohmann::ordered_json j;
        j["start_x"] = start_x;
        j["start_y"] = start_y;
        j["width"] = width;
        j["height"] = height;
        j["depth"] = depth;
        j["quadtree_topleft"] = qd_topleft ? qd_topleft->to_json() : nlohmann::json();
        j["quadtree_topright"] = qd_topright ? qd_topright->to_json() : nlohmann::json();
        j["quadtree_downleft"] = qd_downleft ? qd_downleft->to_json() : nlohmann::json();
        j["quadtree_downright"] = qd_downright ? qd_downright->to_json() : nlohmann::json();
        return std::move(j);
    }

    bool json_write(const char* jsonpath){
        nlohmann::ordered_json j = to_json();
        std::ofstream ofs(jsonpath);
        if (!ofs.is_open()) {
            return false;
        }
        ofs << j.dump(4);
        ofs.close();
        return true;
    }
};

enum class method{ percent, count};

bool cal_percent(char** mask,int sx, int sy, int w, int h, double t){
    int valid_count = 0;
    for(int i=0; i < h; i++){
        for(int j=0; j < w; j++){
            valid_count += mask[i+sy][j+sx];
        }
    }
    return double(valid_count) / (w*h) >= t;
}

bool cal_count(char** mask,int sx, int sy, int w, int h, double t){
    int valid_count = 0;
    for(int i=0; i < h; i++){
        for(int j=0; j < w; j++){
            valid_count += mask[i+sy][j+sx];
        }
    }
    // std::cout<<fmt::format("tl:{},{}; size:{},{}, valid_count:{}, thres:{}\n", sy, sx, h, w, valid_count, t);
    return valid_count >= t;
}

/*
argparse::ArgumentParser sub_quadtree("quadtree");
    sub_quadtree.add_description("create quadtree base on a raster mask with byte datatype.");
    {
        sub_quadtree.add_argument("input")
            .help("raster image (dem), with short or float datatype.");

        sub_quadtree.add_argument("depth")
            .help("quadtree's max depth.")
            .scan<'i',int>();

        sub_quadtree.add_argument("output_jsonpath")
            .help("output jsonpath. ");

        sub_quadtree.add_argument("thres")
            .help("2 pars , the 1st par is method, like 'percent' or 'count', the 2nd par is percent(double, 0~1) or count(int, >0)")
            .nargs(2);        

    }
*/

int create_quadtree(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_maskpath  = args->get<string>("input");
	int max_depth  = args->get<int>("depth");
    std::string output_path  = args->get<string>("output_jsonpath");
    /// 1.占比: "percent" + double; 2.数量: "count" + int
    std::vector<std::string> thres_vec = args->get<std::vector<std::string>>("thres");
    if(thres_vec.size() < 2){
        PRINT_LOGGER(logger, error, "thres_vec.size() < 2 .");
        return -1;
    }

    method iter_method;
    if(thres_vec[0]=="percent")
        iter_method = method::percent;
    else if(thres_vec[0]=="count")
        iter_method = method::count;
    else{
        PRINT_LOGGER(logger, error, "unknown thres method.");
        return -2;
    }

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto dataset = (GDALDataset*)GDALOpen(input_maskpath.c_str(), GA_ReadOnly);
    if(!dataset){
        PRINT_LOGGER(logger, error, "dataset is nullptr.");
        return -3;
    }

    int width = dataset->GetRasterXSize();
    int height = dataset->GetRasterYSize();

    auto band = dataset->GetRasterBand(1);
    auto datatype = band->GetRasterDataType();

    if(datatype != GDT_Byte){
        PRINT_LOGGER(logger, error, "datatype is not byte mask.");
        GDALClose(dataset);
        return -4;
    }

    char* arr = new char[width * height];
    char** ptr = new char*[height];
    for(int i=0; i< height; i++){
        ptr[i] = &arr[i*width];
    }
    band->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);
    GDALClose(dataset);

    quadtree qd_root(0,0, width, height);

    switch (iter_method)
    {
    case method::percent:{
        double percent = std::stod(thres_vec[1]);
        qd_root.iter(max_depth, percent, ptr, cal_percent);
        }break;
    case method::count:{
        double count = std::stod(thres_vec[1]);
        qd_root.iter(max_depth, count, ptr, cal_count);
        }break;

    }
    delete[] arr;
    delete[] ptr;

    std::cout<<"tree_count: "<<qd_root.tree_count()<<std::endl;

    if(!qd_root.json_write(output_path.c_str())){
        PRINT_LOGGER(logger, error, "qd_root.json_write failed.");
        return -5;
    }

    qd_root.close();

    PRINT_LOGGER(logger, info, "create_quadtree finished.");
    return 1;
}


