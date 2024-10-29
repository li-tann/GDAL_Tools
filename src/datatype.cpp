#include "datatype.h"
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <gdal_priv.h>

namespace fs = std::filesystem;

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

void strSplits(std::string input, std::vector<std::string>& output, std::vector<std::string> splitters, bool clearVector)
{
    if(clearVector)
        output.clear();
    std::vector<std::string> splitted;
    splitted.push_back(input);
    size_t idx_start = 0, idx_end = 1;
    for(auto splitter : splitters)
    {
        size_t tmp_idx_start = idx_start;
        idx_start = splitted.size();
        for(int i = tmp_idx_start; i < idx_end; i++){
            strSplit(splitted[i], splitted, splitter, false);
        }
        idx_end = splitted.size();
    }

    for(int i = idx_start; i <  idx_end; i++){
        output.push_back(splitted[i]);
    }
    splitted.clear();
}

double spend_time(decltype (std::chrono::system_clock::now()) start)
{
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double second = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    return second;
}

double spend_time(decltype (std::chrono::system_clock::now()) start, decltype (std::chrono::system_clock::now()) end)
{
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double second = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
    return second;
}


void strTrim(string &s)
{
	int index = 0;
	if(!s.empty())
	{
		while( (index = s.find(' ',index)) != string::npos)
		{
			s.erase(index,1);
		}
	}
}

rgba::rgba():
    red(0), green(0), blue(0), alpha(0) {

    };
rgba::rgba(int r, int g, int b, int a) :
    red(r > 255 ? 255 : (r < 0 ? 0 : r)), green(g > 255 ? 255 : (g < 0 ? 0 : g)),
    blue(b > 255 ? 255 : (b < 0 ? 0 : b)),alpha(a > 255 ? 255 : (a < 0 ? 0 : a)) {

    }

rgba::rgba(int r, int g, int b) :
    red(r > 255 ? 255 : (r < 0 ? 0 : r)),green(g > 255 ? 255 : (g < 0 ? 0 : g)),
    blue(b > 255 ? 255 : (b < 0 ? 0 : b)),alpha(255) {

    }

rgba::rgba(std::string hex){

    auto hex_to_int = [](char c){
        /// 有一个前提是默认输出的字符串是大写, 因为之前已经使用了transform(...,::toupper)
        return (c < 48 ? 0 : (c < 58 ? int(c-'0') : (c < 65 ? 0 : (c < 71 ? 10+int(c-'A') : 0))));
    };

    std::transform(hex.begin(), hex.end(), hex.begin(), ::toupper);
    int offset = (hex[0] == '#') ? 1 : 0;
    if(hex.size() < 6 + offset){
        red = green = blue = alpha = 0;
        return ;
    }
    red   = hex_to_int(hex[offset + 0]) * 16 + hex_to_int(hex[offset + 1]);
    green = hex_to_int(hex[offset + 2]) * 16 + hex_to_int(hex[offset + 3]);
    blue  = hex_to_int(hex[offset + 4]) * 16 + hex_to_int(hex[offset + 5]);
    if(hex.size() > 8 + offset){
        alpha = hex_to_int(hex[offset + 6]) * 16 + hex_to_int(hex[offset + 7]);
    }else{
        alpha = 255;
    }
};

std::string rgba::rgb_to_hex(){
    auto int_to_hex = [](int c)->std::string{
        std::string s;
        c < 10 ? s = std::to_string(c) : s = 'A'+c-10;
        return s;
    };
    std::string s = "#";
    for(int c : {red, green, blue}){
        s += int_to_hex(c / 16);
        s += int_to_hex(c % 16);
    }
    return s;
}

std::string rgba::rgba_to_hex(){
    auto int_to_hex = [](int c)->std::string{
        std::string str;
        c < 10 ? str = std::to_string(c) : str = 'A'+c-10;
        return str;
    };
    std::string s = "#";
    for(int c : {red, green, blue, alpha}){
        s += int_to_hex(c / 16);
        s += int_to_hex(c % 16);
    }
    return s;
}

bool rgba::operator==(rgba src){
    if(red == src.red && blue == src.blue && green == src.green && alpha == src.alpha)
        return true;
    return false;
}

hsv rgba::to_hsv()
{
    return rgb_to_hsv(*this);
}

void rgba::from_hsv(hsv _hsv_)
{
    *this = hsv_to_rgb(_hsv_);
}

hsv::hsv() :
    hue(0), saturation(0), value(0), alpha(0){
        
    }

hsv::hsv(double h, double s, double v) :
    hue(h), saturation(s), value(v), alpha(255){

    }

rgba hsv::to_rgb()
{
    return hsv_to_rgb(*this);
}

void hsv::from_rgb(rgba _rgba_)
{
    *this = rgb_to_hsv(_rgba_);
}

std::map<std::string, std::string> map_with_color_and_name = {
    {"aliceblue","#f0f8ff"},{"antiquewhite","#faebd7"},{"aqua","#00ffff"},{"cyan","#00ffff"},{"aquamarine","#7fffd4"},
    {"azure","#f0ffff"},{"beige","#f5f5dc"},{"bisque","#ffe4c4"},{"black","#000000"},{"blanchedalmond","#ffebcd"},
    {"blue","#0000ff"},{"blueviolet","#8a2be2"},{"brown","#a52a2a"},{"burlywood","#deb887"},{"cadetblue","#5f9ea0"},
    {"chartreuse","#7fff00"},{"chocolate","#d2691e"},{"coral","#ff7f50"},{"cornflowerblue","#6495ed"},{"cornsilk","#fff8dc"},
    {"crimson","#dc143c"},{"darkblue","#00008b"},{"darkcyan","#008b8b"},{"darkgoldenrod","#b8860b"},{"darkgray","#a9a9a9"},
    {"darkgreen","#006400"},{"darkkhaki","#bdb76b"},{"darkmagenta","#8b008b"},{"darkolivegreen","#556b2f"},{"darkorange","#ff8c00"},
    {"darkorchid","#9932cc"},{"darkred","#8b0000"},{"darksalmon","#e9967a"},{"darkseagreen","#8fbc8f"},{"darkslateblue","#483d8b"},
    {"darkslategray","#2f4f4f"},{"darkturquoise","#00ced1"},{"darkviolet","#9400d3"},{"deeppink","#ff1493"},{"deepskyblue","#00bfff"},
    {"dimgray","#696969"},{"dodgerblue","#1e90ff"},{"firebrick (Fire Brick)","#b22222"},{"floralwhite","#fffaf0"},{"forestgreen","#228b22"},
    {"fuchsia","#ff00ff"},{"magenta","#ff00ff"},{"gainsboro","#dcdcdc"},{"ghostwhite","#f8f8ff"},{"gold","#ffd700"},
    {"goldenrod","#daa520"},{"gray","#808080"},{"green","#008000"},{"greenyellow","#adff2f"},{"honeydew","#f0fff0"},
    {"hotpink","#ff69b4"},{"indianred","#cd5c5c"},{"indigo","#4b0082"},{"ivory","#fffff0"},{"khaki","#f0e68c"},
    {"lavender","#e6e6fa"},{"lavenderblush","#fff0f5"},{"lawngreen","#7cfc00"},{"lemonchiffon","#fffacd"},{"lightblue","#add8e6"},
    {"lightcoral","#f08080"},{"lightcyan","#e0ffff"},{"lightgoldenrodyellow","#fafad2"},{"lightgray","#d3d3d3"},{"lightgreen","#90ee90"},
    {"lightpink","#ffb6c1"},{"lightsalmon","#ffa07a"},{"lightseagreen","#20b2aa"},{"lightskyblue","#87cefa"},{"lightslategray","#778899"},
    {"lightsteelblue","#b0c4de"},{"lightyellow","#ffffe0"},{"lime","#00ff00"},{"limegreen","#32cd32"},{"linen","#faf0e6"},{"maroon","#800000"},
    {"mediumaquamarine","#66cdaa"},{"mediumblue","#0000cd"},{"mediumorchid","#ba55d3"},{"mediumpurple","#9370db"},{"mediumseagreen","#3cb371"},
    {"mediumslateblue","#7b68ee"},{"mediumspringgreen","#00fa9a"},{"mediumturquoise","#48d1cc"},{"mediumvioletred","#c71585"},{"midnightblue","#191970"},
    {"mintcream","#f5fffa"},{"mistyrose","#ffe4e1"},{"moccasin","#ffe4b5"},{"navajowhite","#ffdead"},{"navy","#000080"},
    {"oldlace","#fdf5e6"},{"olive","#808000"},{"olivedrab","#6b8e23"},{"orange","#ffa500"},{"yellowgreen","#9acd32"},
    {"orangered","#ff4500"},{"orchid","#da70d6"},{"palegoldenrod","#eee8aa"},{"palegreen","#98fb98"},{"paleturquoise","#afeeee"},
    {"palevioletred","#db7093"},{"papayawhip","#ffefd5"},{"peachpuff","#ffdab9"},{"peru","#cd853f"},{"pink","#ffc0cb"},
    {"plum","#dda0dd"},{"powderblue","#b0e0e6"},{"purple","#800080"},{"red","#ff0000"},{"rosybrown","#bc8f8f"},
    {"royalblue","#4169e1"},{"saddlebrown","#8b4513"},{"salmon","#fa8072"},{"sandybrown","#f4a460"},{"seagreen","#2e8b57"},
    {"seashell","#fff5ee"},{"sienna","#a0522d"},{"silver","#c0c0c0"},{"skyblue","#87ceeb"},{"slateblue","#6a5acd"},
    {"slategray","#708090"},{"snow","#fffafa"},{"springgreen","#00ff7f"},{"steelblue","#4682b4"},{"tan","#d2b48c"},
    {"teal","#008080"},{"thistle","#d8bfd8"},{"tomato","#ff6347"},{"turquoise","#40e0d0"},{"violet","#ee82ee"},
    {"wheat","#f5deb3"},{"white","#ffffff"},{"whitesmoke","#f5f5f5"},{"yellow","#ffff00"}
};

funcrst get_color_from_name(std::string color_name)
{
    auto rst = map_with_color_and_name.find(color_name);
    if(rst == map_with_color_and_name.end()){
        return funcrst(false, "#000000");
    }
    return funcrst(true, rst->second);
}

rgba hsv_to_rgb(hsv _hsv_)
{
    double h = _hsv_.hue;
    double s = _hsv_.saturation;
    double v = _hsv_.value;
    int a = _hsv_.alpha;

    while(h>=360){
        h -= 360;
    }
    int hi = int(floor(h/60));
    double f = h / 60. - hi;
    double p = v *(1-s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1-f) * s);

    switch (hi)
    {
    case 0:
        return rgba(int(v*255), int(t*255), int(p*255), a);
    case 1:
        return rgba(int(q*255), int(v*255), int(p*255), a);
    case 2:
        return rgba(int(p*255), int(v*255), int(t*255), a);
    case 3:
        return rgba(int(p*255), int(q*255), int(v*255), a);
    case 4:
        return rgba(int(t*255), int(p*255), int(v*255), a);
    case 5:
        return rgba(int(v*255), int(p*255), int(q*255), a);
    }
    return rgba(-1, -1, -1, 0);
}

hsv rgb_to_hsv(rgba _rgba_)
{
    hsv dst;
    double r = _rgba_.red / 255.;
    double g = _rgba_.green / 255.;
    double b = _rgba_.blue / 255.;

    std::vector<double> elements = {r, g, b};

    auto cmax = std::max_element(elements.begin(), elements.end());
    auto cmin = std::min_element(elements.begin(), elements.end());

    double delta = *cmax - *cmin;

    if(delta == 0){
        dst.hue = 0;
    }
    else if(*cmax == r){
        dst.hue = 60 * (g - b) / delta;
        dst.hue += (g < b ? 360 : 0);
    }
    else if (*cmax == g){
        dst.hue = 60 * (b - r) / delta + 120;
    }
    else{ ///*cmax == b
        dst.hue = 60 * (r - g) / delta + 240;
    }

    dst.saturation = (*cmax == 0 ? 0 : delta / *cmax) ;
    dst.value = *cmax;
    dst.alpha = _rgba_.alpha;

    return dst;
}

color_map::color_map(const char* color_map_filepath)
{
    fs::path p(color_map_filepath);
    if (p.extension() == ".cpt") {
        load_cpt(color_map_filepath);
    }
    else if (p.extension() == ".cm") {
        load_cm(color_map_filepath);
    }
    else {
        /// do nothing
    }
}

color_map::color_map(const char* color_map_filepath, color_map_type type)
{
    switch (type)
    {
    case color_map_type::cm: {
        load_cm(color_map_filepath);
    }break;
    case color_map_type::cpt: {
        load_cpt(color_map_filepath);
    }break;
    default:
        break;
    }
}

color_map::~color_map()
{
    if (node != nullptr) {
        delete[] node;
    }

    if (color != nullptr) {
        delete[] color;
    }
}

funcrst color_map::is_opened()
{
    std::string error_explain = "open failed, cause: ";
    bool return_bool = true;
    if (node == nullptr) {
        error_explain += "node is nullptr. ";
        return_bool = false;
    }

    if (color == nullptr) {
        error_explain += "color is nullptr. ";
        return_bool = false;
    }

    int node_size = int(_msize(node) / sizeof(*node));
    int color_size = int(_msize(color) / sizeof(*color));
    if (node_size + 1 != color_size) {
        error_explain += fmt::format("node.size({})+1 != color.size({})", node_size, color_size);
        return_bool = false;
    }

    return funcrst(return_bool, return_bool ? "is opened" : error_explain);
}

funcrst color_map::mapping(float node_min, float node_max)
{
    funcrst rst = is_opened();
    if (!rst){
        return funcrst(false, fmt::format("mapping failed, cause there is not open, '{}'.",rst.explain));
    }

    int node_size = int(_msize(node) / sizeof(float));
    if (node_size < 2) {
        return funcrst(false, "node.size < 2, mapping failed.");
    }

    float* temp_node = new float[node_size];

    temp_node[0] = node_min;
    temp_node[node_size-1] = node_min;

    for (int i = 1; i < node_size; i++) {
        temp_node[i] = (node[i] - node[0]) / (node[node_size - 1] - node[0]) * (node_max - node_min) + temp_node[0];
    }

    for (int i = 0; i < node_size; i++) {
        node[i] = temp_node[i];
    }

    delete[] temp_node;
    return funcrst(true,"mapping success.");
}

rgba color_map::mapping_color(float value)
{
    int node_size = int(_msize(node) / sizeof(float));
    if (value <= node[0])return color[0];
    if (value > node[node_size - 1])return color[node_size];
    for (int i = 0; i < node_size - 1; i++) {
        if (value > node[i] && value <= node[i + 1]) {
            return color[i + 1];
        }
    }
    return rgba(0, 0, 0, 0);
}

void color_map::print_colormap()
{
    int node_size =int(_msize(node) / sizeof(float));

    auto rgba_to_str = [](rgba c){
        return fmt::format("{},{},{},{}", c.red, c.green, c.blue, c.alpha);
    };
    for(int i=0; i< node_size; i++)
    {
        std::cout<<fmt::format("color[{}]: {:>20}\n", i, rgba_to_str(color[i]));
        std::cout<<fmt::format("node[{}] : {:<20}\n", i, node[i]);
    }
    std::cout<<fmt::format("color[{}]: {:>20}\n", node_size, rgba_to_str(color[node_size]));
}

void color_map::load_cm(const char* cm_path)
{
    /// type:
    /// r g b
    /// r g b
    /// ...

    std::ifstream ifs(cm_path);
    if (!ifs.is_open()) {
        return ;
    }
    std::vector<std::string> texts;
    std::string str;
    while (std::getline(ifs, str)) {
        if (str[0] == '#')
            continue;
        texts.push_back(str);
    }
    ifs.close();

    if (texts.size() == 0) {
        return;
    }

    node = new float[texts.size()];
    color = new rgba[texts.size() + 1];

    for (int i = 0; i < texts.size(); i++) {
        std::vector<std::string> vec_splited;
        strSplit(texts[i], vec_splited, " ");
        if (vec_splited.size() < 3)
            continue;

        node[i] = i;
        color[i].red = stoi(vec_splited[0]);
        color[i].green = stoi(vec_splited[1]);
        color[i].blue = stoi(vec_splited[2]);
        color[i].alpha = 255;
    }
    color[texts.size()] = color[texts.size() - 1];

    return;
}
void color_map::load_cpt(const char* cpt_path)
{
    /// type:
    /// # COLOR_MODEL = RGB or hsv
    /// # HARD_HINGE/SOFT_HINGE/CYCLIC/NULL
    /// value r g b value r g b, spliter is " ", "/", or "-"
    /// ...
    
    std::ifstream ifs(cpt_path);
    if (!ifs.is_open()) {
        return;
    }
    std::string str;
    std::vector<std::string> cpt_list;
    while (std::getline(ifs, str)) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        cpt_list.push_back(str);
    }
    ifs.close();

    // std::cout<<"cpt_list.size:"<<cpt_list.size()<<std::endl;

    bool is_rgb =true;
    std::string str_null = "";  /// null_color可能用不到
    int line_start = -1, line_end = -1;
    for(int i=0; i<cpt_list.size(); i++)
    {
        /// 确认RGB or HSV
        if(cpt_list[i].find("color_model") != std::string::npos){
            if(cpt_list[i].find("hsv") != std::string::npos)
                is_rgb = false;
        }

        // /// null_color可能用不到
        // if(cpt_list[i][0] == 'n'){
        //     std::vector<std::string> tmp_str_vec;
        //     strSplit(cpt_list[i], tmp_str_vec, " ");
        //     if(tmp_str_vec.size()>1)
        //         str_null = tmp_str_vec[1];
        // }

        /// 确认记录RGB(/HSV)值的起止区间行数
        if(cpt_list[i][0] != '#' && line_start == -1){
            line_start = i;
        }

        if(line_start != -1 && line_end == -1 && ( i + 1  == cpt_list.size() || 
            (cpt_list[i+1][0] == '#' || cpt_list[i+1][0] == 'f' || cpt_list[i+1][0] == 'b' || cpt_list[i+1][0] == 'n'))){
            line_end = i;
            // std::cout<<"line_end:"<<cpt_list[i]<<std::endl;
        }
    }
    std::string spliter = is_rgb ? "/" : "-";

    // std::cout<<"is_rgb:"<<(is_rgb ? "true" : "false")<<std::endl;
    // std::cout<<fmt::format("line_start:[{}], line_end:[{}]\n", line_start, line_end);

    auto is_colorname = [](std::string str)->bool{
        if(str[0] < 'a' || str[0] > 'z'){
            return false;
        }
        return true;
    };
    auto colorname_to_rgb = [](std::string colorname, rgba& color)->bool{
        auto rst = get_color_from_name(colorname);
        if(!rst){
            return false;
        }
        color = rgba(rst.explain);
        return true;
    };

    auto colorstr_to_rgb = [is_rgb](std::string colorstr, std::string spliter, rgba& color)->bool{
        std::vector<std::string> tmp_colorlist;
        strSplit(colorstr, tmp_colorlist, spliter);
        if(tmp_colorlist.size() < 3){
            return false;
        }
        if(!is_rgb){
            float h = std::stof(tmp_colorlist[0]);
            float s = std::stof(tmp_colorlist[1]);
            float v = std::stof(tmp_colorlist[2]);
            rgba _rgb_ = hsv_to_rgb(hsv(h,s,v));
            color = _rgb_;
        }
        else{
            color.red = std::stoi(tmp_colorlist[0]);
            color.green = std::stoi(tmp_colorlist[1]);
            color.blue = std::stoi(tmp_colorlist[2]);
        }
        
        
        return true;
    };

    size_t splited_size; /// line_start行, 的列数
    {
        std::vector<std::string> tmp_strlist;
        strSplits(cpt_list[line_start], tmp_strlist, {"\t", " "});
        splited_size = tmp_strlist.size();
    }
    std::cout<<fmt::format("line_start:'{}', line_end:'{}'\n", line_start, line_end);
    std::cout<<"cpt_list[line_start]:"<<cpt_list[line_start]<<std::endl;
    std::cout<<"splited_size:"<<splited_size<<std::endl;

    if(splited_size == 2){
        node = new float[line_end - line_start + 1];
        color = new rgba[line_end - line_start + 2];
        for(int i=line_start; i<=line_end; i++)
        {
            std::vector<std::string> tmp_strlist;
            strSplits(cpt_list[i], tmp_strlist, {"\t", " "});
            float node_i = std::stof(tmp_strlist[0]);
            rgba color_i;
            if(is_colorname(tmp_strlist[1])){
                if(!colorname_to_rgb(tmp_strlist[1], color_i)){
                    /// explain:   eror_str = "...unknown colorname:{}..., tmp_strlist[1]";
                    delete[] node; node = nullptr;
                    delete[] color; color = nullptr;
                    return;
                }
            }else{
                if(!colorstr_to_rgb(tmp_strlist[1], spliter, color_i)){
                    /// explain
                    delete[] node; node = nullptr;
                    delete[] color; color = nullptr;
                    return ;
                }
            }
            node[i - line_start] = node_i;
            color[i - line_start] = color_i;
            if(i == line_end){
                color[i - line_start + 1] = color_i;
            }
        }
    }
    else if(splited_size == 4)
    {
        node = new float[line_end - line_start + 2];
        color = new rgba[line_end - line_start + 3];
        for(int i=line_start; i<=line_end; i++)
        {
            std::vector<std::string> tmp_strlist;
            strSplits(cpt_list[i], tmp_strlist, {" ", "\t"});
            float node_i = std::stof(tmp_strlist[0]);
            rgba color_i;
            if(is_colorname(tmp_strlist[1])){
                if(!colorname_to_rgb(tmp_strlist[1], color_i)){
                    /// explain:   eror_str = "...unknown colorname:{}..., tmp_strlist[1]";
                    delete[] node; node = nullptr;
                    delete[] color; color = nullptr;
                    return;
                }
            }else{
                if(!colorstr_to_rgb(tmp_strlist[1], spliter, color_i)){
                    /// explain
                    delete[] node; node = nullptr;
                    delete[] color; color = nullptr;
                    return ;
                }
            }
            node[i - line_start] = node_i;
            color[i - line_start + 1] = color_i;
            if(i == line_start){
                color[i - line_start] = color_i;
            }
            if(i == line_end){
                rgba color_i2;
                if(is_colorname(tmp_strlist[3])){
                    if(!colorname_to_rgb(tmp_strlist[3], color_i2)){
                        /// explain:   eror_str = "...unknown colorname:{}..., tmp_strlist[1]";
                        delete[] node; node = nullptr;
                        delete[] color; color = nullptr;
                        return;
                    }
                }else{
                    if(!colorstr_to_rgb(tmp_strlist[3], spliter, color_i2)){
                        /// explain
                        delete[] node; node = nullptr;
                        delete[] color; color = nullptr;
                        return ;
                    }
                }
                node[i - line_start + 1] = std::stof(tmp_strlist[2]);
                color[i - line_start + 2] = color_i2;
            }
        }
    }
    else{
        /// explain
        return;
    }

    return;
    /// 以空格为分隔符, 可能出现以下几种情况
    /// 1. 0.1  255/255/255 0.2 0/0/255     -> size == 4            常规
    /// 2. 0.1  white       0.2 0/0/255     -> size == 4            abyss.cpt
    /// xxxxx 3. 0.1  255 255 255 0.2 0 0 255     -> size == 8
    /// xxxxx 4. 0.1  white       0.2 0 0 255     -> size == 6
    /// xxxxx 5. 0.1  255 255 255 0.2 blue        -> size == 6
    /// 6. 0.1  white       0.2 blue        -> size == 4
    /// 7. 0.1 white                        -> size == 2            categorical.cpt
    /// 8. 0.1 255/255/255                  -> size == 2            actonS.cpt  bamakoS.cpt
    /// 9. 0.1 255 255 255                  -> size == 4
    /// 其中1.2.6.可使用同种方式处理, 3.使用另一种方式处理, 但4.5.有点麻烦, 需要判断第一个rgb或第二个rgb哪个使用单词代替
    /// Note 1.: hsv: 0.1 0-1-1 1 360-1-1
    /// Note 2.: 涛哥提供的最新cpt中, 不再有以空格为分隔符的情况, 所以3.4.5.9.四种情况都不存在

    /// cpt parser

    ///  hsv对应分割符是"-", rgb的分割符大部分是"/", 但也见过空格" "
    /// 有三行数字B F N 分别对应background, foreground, nan
    /// 确认rgb or hsv

    /// 获取有效数据区间(不带#的行数)

}


funcrst cal_stretched_minmax(GDALRasterBand* rb, int histogram_size, double stretch_rate, double& min, double& max)
{
	CPLErr cplerr;
	double minmax[2];
	cplerr = rb->ComputeRasterMinMax(false, minmax);
	if (cplerr == CE_Failure) {
		return funcrst(false, "rb.ComputeRasterMinMax failed.");
	}

	GUIntBig* histgram_result = new GUIntBig[histogram_size];
	cplerr = rb->GetHistogram(minmax[0], minmax[1], histogram_size, histgram_result, FALSE, FALSE, GDALDummyProgress, nullptr);
	if (cplerr == CE_Failure) {
		delete[] histgram_result;
		return funcrst(false, "rb.GetHistogram failed.");
	}

	GUIntBig* histgram_accumulate = new GUIntBig[histogram_size];
	histgram_accumulate[0] = histgram_result[0];
	for (int i = 1; i < histogram_size; i++) {
		histgram_accumulate[i] = histgram_accumulate[i - 1] + histgram_result[i];
	}
	/// 换算成百分比
	double* histgram_accumulate_percent = new double[histogram_size];
	bool update_min{ false }, update_max{ false };
    int index_min{-1}, index_max{-1};
	for (int i = 0; i < histogram_size; i++) {
		histgram_accumulate_percent[i] = 1. * histgram_accumulate[i] / histgram_accumulate[histogram_size - 1];
		if (i == 0)continue;
		if ((histgram_accumulate_percent[i - 1] <= stretch_rate || i == 1) && histgram_accumulate_percent[i] >= stretch_rate) {
			min = minmax[0] + (minmax[1] - minmax[0]) / histogram_size * (i - 1);
            index_min = i;
			update_min = true;
		}
		if (histgram_accumulate_percent[i - 1] <= 1 - stretch_rate && histgram_accumulate_percent[i] >= 1 - stretch_rate) {
			max = minmax[0] + (minmax[1] - minmax[0]) / histogram_size * i;
            index_max = i;
			update_max = true;
		}
	}

	delete[] histgram_result;
	delete[] histgram_accumulate;

	if (!update_min) {
		return funcrst(false, "min is not be updated.");
	}

	if (!update_max) {
		return funcrst(false, "max is not be updated.");
	}

	return funcrst(true, "cal_stretched_minmax success.");
}

/// egm2008

egm2008::egm2008(/* args */)
{
}

egm2008::~egm2008()
{
    if(arr == nullptr)
        delete[] arr;
}

funcrst egm2008::init(std::string path)
{
    ifstream ifs(path, ifstream::binary);
    if(!ifs.is_open()){
        lastError = "init, ifs.is_open return false.";
        return funcrst(false, lastError);
    }

    auto start = std::chrono::system_clock::now();

    std::string str;
    width = 0;
    height = 0;
    float value;
    zero_number = 0;
    bool stat_width = false;   
    while (ifs.read((char*)&value, 4)) { //一直读到文件结束
        if(value == 0){
            ++zero_number;
            if(zero_number == 1){
                stat_width = true;
            }
            if(zero_number == 2){
                stat_width = false;
            }
        }
        else{
            if(stat_width){
                ++width;
            }
        }
    }
    ifs.clear();
    ifs.seekg(0,ios::beg);
    
    cout<<"zero_number:"<<zero_number<<endl;

    double seconds = spend_time(start);
    cout<<"init, spend_time: "<<seconds<<"s."<<endl;

    height = zero_number / 2;
    if(height != (width / 2 + 1)){
        lastError = fmt::format("init, height({}) != width({}) / 2 + 1\n",height, width);
        return funcrst(false, lastError);
    }

    spacing = double(360) / width; 

    egm_gt[0] = 0;
    egm_gt[1] = spacing;
    egm_gt[2] = 0;
    egm_gt[3] = 90;
    egm_gt[4] = 0;
    egm_gt[5] = -spacing;

    /// 计算完height 和 width 后, 可以定义数组长度

    arr = new float[height * width];
    int num = 0;
    while (ifs.read((char*)&value, 4)){ //一直读到文件结束
        if(value == 0){
            continue;
        }
        else{
            if(num >= height * width){
                lastError = fmt::format("init, the number({}) of non-zero is more than height({}) * width({})", num, height, width);
                return funcrst(false,lastError);
            }
            arr[num] = value;
            ++num;
        }
    }

    cout<<"2se, num:"<<num<<endl;
    ifs.close();
    return funcrst(true, "init, success.");
}

xy egm2008::cal_image_pos(double lon, double lat)
{
    /// 原数据中, 经度的取值范围是0~360, 所以需要对lon进行一定处理
    /// 默认输入的数据时-180~180, -90~90之间, 需要在外面对异常数据进行限制
    if(lon < 0)
        lon += 360;

    xy dst;
    dst.y = (lat - egm_gt[3]) / egm_gt[5]; 
    dst.x = (lon - egm_gt[0]) / egm_gt[1];
    return dst;
}

long egm2008::cal_off(size_t row, size_t col)
{
    if(row > height || col > width){
        return -1;
    }
    return row * (width + 2) + col + 1;
}


float egm2008::calcluate_height_anomaly_single_point(double lon, double lat)
{
    xy image_pos = cal_image_pos(lon, lat);
    // cout<<"cal_image_pos: "<<image_pos.to_str()<<endl;
    
    /// 数组描述的是一个圆柱体, 经度到达尾部时, 下一列数据可以取0 
    int pos_left = int(image_pos.x);
    int pos_right = (pos_left == width - 1) ? 0 : pos_left + 1;
    
    int pos_top = int(image_pos.y);
    if(pos_top == -90){
        pos_top = -89;
    }
    int pos_down  = pos_top + 1;

    // cout<<"pos_left: "<<pos_left<<endl;
    // cout<<"pos_right: "<<pos_right<<endl;
    // cout<<"pos_top: "<<pos_top<<endl;
    // cout<<"pos_down: "<<pos_down<<endl;

    float value_topleft = arr[pos_top * width + pos_left];
    float value_topright = arr[pos_top * width + pos_right];
    float value_downleft = arr[pos_down * width + pos_left];
    float value_downright = arr[pos_down * width + pos_right];

    // cout<<"value_topleft: "<<value_topleft<<endl;
    // cout<<"value_topright: "<<value_topright<<endl;
    // cout<<"value_downleft: "<<value_downleft<<endl;
    // cout<<"value_downright: "<<value_downright<<endl;


    xy delta(image_pos.x - pos_left, image_pos.y - pos_top);
    float value =   (1 - delta.y) * (1 - delta.x) * value_topleft + 
                    (1 - delta.y) * (delta.x) * value_topright +
                    (delta.y) * (1 - delta.x) * value_downleft + 
                    (delta.y) * (delta.x) * value_downright;
    
    return value;
}

funcrst egm2008::write_height_anomaly_txt(const char* input_filepath, const char* output_filepath, reference_elevation_system sys)
{
    ifstream ifs(input_filepath);
    if(!ifs.is_open()){
        return funcrst(false, "write_height_anomaly_txt, ifs(input) open failed.");
    }

    ofstream ofs(output_filepath);
    if(!ofs.is_open()){
        return funcrst(false, "write_height_anomaly_txt, ofs(output) open failed.");
    }

    auto get_corrected_height = [](float src_height, float abnormal_height, reference_elevation_system sys){
        float dst = src_height;
        switch (sys)
        {
        case normal:
            dst = src_height + abnormal_height;
            break;
        case geodetic:
            dst = src_height - abnormal_height;
            break;
        default:
            break;
        }
        return dst;
    };

    string str;
    while (getline(ifs,str))
    {
        vector<string> splited;
        ofs<<str;
        strSplit(str,splited,",");
        if(splited.size() < 3)
            continue;
        
        float lon = stof(splited[0]);
        float lat = stof(splited[1]);
        float hei = stof(splited[2]);
        if(lon > 180 || lon < -180 || lat > 90 || lat < -90){
            ofs<<", the coordinate is not a valid value."<<endl;
            continue;
        }

        float abnormal = calcluate_height_anomaly_single_point(lon, lat);
        float corrected_height = get_corrected_height(hei, abnormal, sys);
        ofs<<fmt::format(", {}, {}", abnormal, corrected_height)<<std::endl;
    }

    ifs.close();
    ofs.close();

    return funcrst(true, "write_height_anomaly_txt, success.");
    
}

funcrst egm2008::write_height_anomaly_image(int interped_height, int interped_width, double interped_gt[], float* height_anomaly_arr)
{
    for(int i=0; i<interped_height; ++i)
    {
        for(int j=0; j<interped_width; ++j)
        {
            double lon = interped_gt[0] + interped_gt[1] * j;
            double lat = interped_gt[3] + interped_gt[5] * i;
            
            if(lon > 180 || lon < -180 || lat > 90 || lat < -90){
                height_anomaly_arr[i*interped_width + j] = NAN; 
            }else{
                height_anomaly_arr[i*interped_width + j] = calcluate_height_anomaly_single_point(lon, lat);
            }
        }
    }

    return funcrst(true, "write_height_anomaly_image, success.");
}


for_loop_timer::for_loop_timer(size_t for_loop_num, void(*func)(size_t, size_t, size_t))
{
    m_for_look_number = for_loop_num;
    info_print = func;
    m_start_time = std::chrono::system_clock::now();
}

void for_loop_timer::update()
{
    if (m_current >= m_for_look_number)
        return;
    ++m_current;
    auto spend_time_ = spend_time(m_start_time);
    size_t remain_sceond = spend_time_ / m_current * (m_for_look_number - m_current);
    info_print(m_current, m_for_look_number, remain_sceond);
}

void for_loop_timer::update_percentage()
{
    size_t current_percentage = ++m_current * 100 / m_for_look_number;
    if(current_percentage <= m_percentage){
        return; /// 不需要重复输出
    }
    m_percentage = current_percentage;
    auto spend_time_ = spend_time(m_start_time);
    size_t remain_sceond = spend_time_ / m_current * (m_for_look_number - m_current);
    info_print(m_current, m_for_look_number, remain_sceond);
}