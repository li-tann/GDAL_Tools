#ifndef TEMPLATE_BINARY_IMG_IO
#define TEMPLATE_BINARY_IMG_IO

#include <iostream>
#include <fstream>

/// 还没有测试

/// arr 不需要定义长度
template<typename _Ty>
bool load_binary_to_arr(const char* binary_imgpath, int width, int height, _Ty* arr)
{
    if(arr != nullptr){
        delete[] arr;
        arr = new _Ty[width * height];
    }

    ifstream ifs(binary_imgpath,ifstream::binary);
    if(!ifs.is_open()){
        return false;
    }

    size_t sizeof_Ty = sizeof(_Ty);
    int num = 0;
    _Ty val_init;
    while (ifs.read((char*)&val_init, sizeof_Ty)) { //一直读到文件结束
        //高低位字节变换
        _Ty val_trans;
        for(int i=0; i< sizeof_Ty; i++)
        {
            int j = sizeof_Ty - 1 - i;
            ((char*)&val_trans)[j] = ((char*)&val_init)[i];
        }
        arr[num++] = val_trans;
    }
}

template<typename _Ty>
inline _Ty binary_swap(_Ty src)
{
    _Ty dst;
    size_t src_size = sizeof(_Ty);
    unsigned char* src_uc = static_cast<unsigned char*>(&src);
    unsigned char* dst_uc = static_cast<unsigned char*>(&dst);
    for (size_t i = 0; i < src_size; i++) {
        dst_uc[i] = src_uc[src_size - 1 - i];
    }
    return dst;
}


#endif //TEMPLATE_BINARY_IMG_IO
