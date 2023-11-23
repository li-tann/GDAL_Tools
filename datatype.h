#ifndef DATATYPE_H
#define DATATYPE_H

#include <vector>
#include <string>
#include <complex>
#include <chrono>

#include <gdal_priv.h>

using namespace std;

#define MAIN_INIT(EXE_NAME)															\
	auto start = chrono::system_clock::now();										\
    GDALAllRegister();																\
    string msg;																		\
    auto my_logger = spdlog::basic_logger_mt(EXE_NAME, EXE_PLUS_FILENAME("txt"));	\
    auto return_msg = [my_logger](int rtn, string msg){								\
        my_logger->info(msg);														\
		spdlog::info(msg);															\
        return rtn;																	\
    };

struct funcrst{
    funcrst() {};
    funcrst(bool b, std::string s) { result = b; explain = s; }
    bool result{ false };
    std::string explain{ "" };
    operator bool() { return result; }
};


void strSplit(std::string input, std::vector<std::string>& output, std::string split, bool clearVector = true);
double spend_time(decltype (std::chrono::system_clock::now()) start);
double spend_time(decltype (std::chrono::system_clock::now()) start, decltype (std::chrono::system_clock::now()) end);
void strTrim(string &s);


template<typename _Ty>
inline size_t dynamic_array_size(_Ty* array)
{
    return _msize(array) / sizeof(*array);
}

/// @brief RGBA
/// r,g,b: 0~255, a:0~255(0是全透明, 255是实心)
struct rgba {
	int red, green, blue, alpha;
	rgba():red(0), green(0), blue(0), alpha(0) {};
	rgba(int r, int g, int b, int a) :
		red(r > 255 ? 255 : (r < 0 ? 0 : r)), 
		green(g > 255 ? 255 : (g < 0 ? 0 : g)),
		blue(b > 255 ? 255 : (b < 0 ? 0 : b)),
		alpha(a > 255 ? 255 : (a < 0 ? 0 : a)) {}
	rgba(int r, int g, int b) :
		red(r > 255 ? 255 : (r < 0 ? 0 : r)),
		green(g > 255 ? 255 : (g < 0 ? 0 : g)),
		blue(b > 255 ? 255 : (b < 0 ? 0 : b)),
		alpha(255) {}
};

class color_map_v2
{
public:
	color_map_v2(const char* color_map_filepath);
	~color_map_v2();

	/// @brief  如果node, color 数组异常, 返回false, 否则为true
	/// @return 
	funcrst is_opened();

	/// @brief 等比例映射, 修改颜色表中的node, cm + mapping 可以约等于cpt
	funcrst mapping(float node_min, float node_max);

	/// @brief 输入value值, 匹配对应的rgba
	rgba mapping_color(float);

	float* node = nullptr;
	rgba* color = nullptr;

	/// rgba.size = node.size + 1, cause:
	///			  node[0]		node[1]			node[2]		...		node[n-1]		node[n]
	/// 	color[0]		color[1]		color[2]	...		color[n-1]		color[n]		color[n+1]
	/// color[0]: out_of_range(left) ; 
	/// color[n+1]: out_of_range(right)
	/// 向上兼容关系, 即 if( node[0] < value  && value <= node[1] ) color = color[1]
	 
	void print_colormap();

private:
	void load_cm(const char* cm_path);
	void load_cpt(const char* cpt_path);
};

/// @brief 通过创建直方图, 对影像进行百分比拉伸, 并获取拉伸后的最值
/// @param rb RasterBand, 图像的某个波段
/// @param histogram_size 直方图长度, 通常为100~256
/// @param stretch_rate 拉伸比例, 通常为 0.02
/// @param min 要输出的最小值
/// @param max 要输出的最大值
/// @return 
funcrst cal_stretched_minmax(GDALRasterBand* rb, int histogram_size, double stretch_rate, double& min, double& max);


struct err_hei_image{
	double start_lon, start_lat;
	size_t width, height;
	double spacing;
	float* arr;
};

struct xy{
	xy():x(0),y(0){}
	xy(double x_, double y_):x(x_),y(y_){}
	string to_str(){
		return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
	}
	double x, y;
};


///  10′*10′的EGM文件, 有1081(180*6+1)行和2160(360*6)列   (这里的列数不计算每行起止处的两个0)
///  以此类推, 1′*1′的EGM文件, 有10801(180*60+1)行21600(360*60)列
class egm2008
{
public:
	egm2008();
	~egm2008();

	std::string lastError;

	funcrst init(std::string path);

	/// 返回某行列号对应在二进制中的偏移量, 如果输入的行列号超过宽高, 则返回-1

	enum point_type{rc_integer, row_integer, col_integer, none_integer};	
	/// 四种点类型, 对应四种插值方式: 整数可以直接取值, 行整数或列整数使用线性插值, 非整数使用双线性插值

	/// 计算经纬度的对应的像平面坐标(行列号, )
	xy cal_image_pos(double lon, double lat);
	
	long cal_off(size_t row, size_t col);

	
	/// 单点计算高程异常值
	float calcluate_height_anomaly_single_point(double lon, double lat);

	funcrst write_height_anomaly_txt(const char* input_filepath, const char* output_filepath);

	/// 输入宽高和六参数, 可以确定一景DEM的基本信息, 计算对应位置的高程异常值, 并输出到height_anomaly_arr中
	funcrst write_height_anomaly_image(int height, int width, double gt[], float* height_anomaly_arr);


	
public:
	
	size_t width;		/// 有效宽, 0 -> 360, 长度没有考虑两端的0, 实际计算偏移量时, 要考虑到每一行都存在的两个0
	size_t height;		/// 高, 90 -> -90
	double spacing;		/// 分辨率, 由于标准EGM文件, 这两个分辨率应该相同, 所以值留了一个spacing
	size_t zero_number;	/// 统计二进制文件中, 0的格式, 如果是标准数据的话, zero_number / 2 == height
	float* arr = nullptr;			/// 去除0之后的二进制数组
	double egm_gt[6];
};

template<typename type>
inline type swap(type src)
{
	type dst;
	size_t src_size = sizeof(type);
	/*unsigned char* src_uc = static_cast<unsigned char*>(&src);
	unsigned char* dst_uc = static_cast<unsigned char*>(&dst);*/
	unsigned char* src_uc = (unsigned char*)(&src);
	unsigned char* dst_uc = (unsigned char*)(&dst);
	for (size_t i = 0; i < src_size; i++) {
		dst_uc[i] = src_uc[src_size - 1 - i];
	}
	return dst;
}



#endif

