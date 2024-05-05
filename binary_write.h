#include "datatype.h"

/// @brief 可以逐行生成binary + vrt的类, 用法是先init定义类, 然后用array_to_bin按顺序的写二进制文件(注意不要用并行打乱存储顺序), 最后用print_vrt生成vrt文件
/// @tparam _Ty 只支持float, cpx_float, short, cpx_short, 其余类型在init时会报错
template<typename _Ty>
class binary_write
{
public:
    binary_write(){}
    ~binary_write(){
        if(ofs.is_open())
            ofs.close();
        if(m_looper_time != nullptr)
            delete[] m_looper_time;
    }

    funcrst init(const char* binary_filepath, int height, int width, ByteOrder_ byte_order)
    {
        using std::ios;
        funcrst rst;
        fs::path binary_path(binary_filepath);

        m_height = height;
        m_width = width;
        m_bin_filename = binary_path.filename().string();
        m_bin_filepath = binary_path.string();
        m_vrt_filepath = binary_path.string() + ".vrt";

        m_byteorder = byte_order;

        if (std::is_same<_Ty, float>::value)
            m_datatype = GDT_Float32;
        else if (std::is_same<_Ty, std::complex<float>>::value)
            m_datatype = GDT_CFloat32;
        else if (std::is_same<_Ty, short>::value)
            m_datatype = GDT_Int16;
        else if (std::is_same<_Ty, std::complex<short>>::value)
            m_datatype = GDT_CInt16;
        else if (std::is_same<_Ty, double>::value)
            m_datatype = GDT_Float64;
        else if (std::is_same<_Ty, std::complex<double>>::value)
            m_datatype = GDT_CFloat64;
        else if (std::is_same<_Ty, int>::value)
            m_datatype = GDT_Int32;
        else if (std::is_same<_Ty, std::complex<int>>::value)
            m_datatype = GDT_CInt32;
        else if (std::is_same<_Ty, unsigned char>::value)
            m_datatype = GDT_Byte;
        else
            return funcrst(false, "print_vrt, unknown type of arr");

        ofs.open(m_bin_filepath, ios::binary);
        if(!ofs.is_open()){
            return funcrst(false, "ofs open failed.");
        }

        /// for_looper_time
        m_looper_time = new for_loop_timer(height * width, [](size_t current, size_t total, size_t remain) {
            std::string info = fmt::format("\rbinary_write progress: {}/{}, remained time: {}s.", current, total, remain);
            std::cout << info;
        });
        return funcrst(true, "binary_write::init success.");
    }

    /// @brief 使用的是顺序存储模式, 所以要避免多任务并行, 以及非正常顺序(倒序, 乱序等)的存储方式
    /// @param arr 数组
    /// @param length 数组长度
    void array_to_bin(_Ty* arr, size_t length)
    {
        if (m_byteorder == ByteOrder_::MSB)
        {
            /// 数组存储格式的转换(从小端转换到大端存储)
            if (std::is_same<_Ty, float>::value || std::is_same<_Ty, short>::value)
            {
                _Ty val;
                for (int i = 0; i < length; i++) {
                    val = swap(arr[i]);
                    ofs.write((char*)(&val), sizeof(val));
                }
            }
            else if (std::is_same<_Ty, std::complex<short>>::value) { ///complex short
                short* arr_st = (short*)(arr);
                short val;
                for (size_t i = 0; i < 2 * length; i++) {
                    val = swap(arr_st[i]);
                    ofs.write((char*)(&val), sizeof(val));
                }
            }
            else if(std::is_same<_Ty, std::complex<float>>::value) { /// complex float
                float* arr_flt = (float*)(arr);
                float val;
                for (size_t i = 0; i < 2*length; i++) {
                    val = swap(arr_flt[i]);
                    ofs.write((char*)(&val), sizeof(val));
                }
            }
            else {
                return ;
            }
        }
        else {
            /// 如果不需要转换到大端存储模式, 则直接逐个写二进制文件即可
            for (int i = 0; i < length; i++) {
                ofs.write((char*)(&arr[i]), sizeof(_Ty));
            }
        }
        m_looper_time->m_current += length - 1;
        m_looper_time->update_percentage(); /// 先从外部让m_current加上length-1, 在update时++m_current就正常了
    }

    /// 需要二进制文件完成后再执行
    funcrst print_vrt()
    {
        using std::ios;
        funcrst rst;

        int pixel_offset = sizeof(_Ty);
        int line_offset = m_width * pixel_offset; 
        std::string str_byte_order = (m_byteorder == ByteOrder_::MSB) ? "MSB" : "LSB";


        GDALAllRegister();
        CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
        GDALDriver* driver_vrt = GetGDALDriverManager()->GetDriverByName("VRT");
        GDALDataset* ds_out = driver_vrt->Create(m_vrt_filepath.c_str(), m_width, m_height, 0, m_datatype, NULL);
        if (!ds_out) {
            return funcrst(false, "print_vrt, ds_out is nullptr.");
        }

        /// 生成配套的vrt文件
        char** papszOptions = NULL;
        papszOptions = CSLAddNameValue(papszOptions, "subclass", "VRTRawRasterBand"); // if not specified, default to VRTRasterBand
        papszOptions = CSLAddNameValue(papszOptions, "SourceFilename", m_bin_filename.c_str()); // mandatory
        papszOptions = CSLAddNameValue(papszOptions, "ImageOffset", "0"); // optional. default = 0
        papszOptions = CSLAddNameValue(papszOptions, "PixelOffset", std::to_string(pixel_offset).c_str()); // optional. default = size of band type
        papszOptions = CSLAddNameValue(papszOptions, "LineOffset", std::to_string(line_offset).c_str()); // optional. default = size of band type * width
        papszOptions = CSLAddNameValue(papszOptions, "ByteOrder", str_byte_order.c_str()); // optional. default = machine order
        papszOptions = CSLAddNameValue(papszOptions, "relativeToVRT", "true"); // optional. default = false
        ds_out->AddBand(m_datatype, papszOptions);
        CSLDestroy(papszOptions);

        delete ds_out;
        return funcrst(true, "print_vrt success.");
    }

    void close(){
        if(ofs.is_open())
            ofs.close();
        std::cout<<std::endl;
    }

    public:
        std::ofstream ofs;
        ByteOrder_ m_byteorder;
        for_loop_timer* m_looper_time = nullptr;
        size_t m_width, m_height;
        GDALDataType m_datatype;
        std::string m_bin_filename;
        std::string m_bin_filepath;
        std::string m_vrt_filepath;

};