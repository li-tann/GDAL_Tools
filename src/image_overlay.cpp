#include "raster_include.h"

#include <omp.h>

/// @brief 普通透明度混合, 融合后的图像看起来是前景图像和背景图像的平滑过渡
/// @param up 
/// @param low 
/// @return 
rgba normal_alpha_blending(rgba& up, rgba& low);

/// @brief 预乘透明度, 这种方法在颜色计算之前预先将颜色值乘以透明度，避免混合过程中的舍入误差，通常效果与普通透明度混合类似，但在某些情况下能提供更好的结果。
/// @param up 
/// @param low 
/// @return 
rgba premultiplied_alpha_blending(rgba& up, rgba& low);

/// @brief 遮罩混合, 使用遮罩图像来控制前景和背景的混合比例，遮罩图像的透明度值决定前景图像和背景图像的融合程度。
/// @param up 
/// @param low 
/// @return 
rgba mask_blending(rgba& up, rgba& low);

/// @brief 加法混合, 前景和背景的颜色值相加，用于产生发光效果，融合后的图像通常更亮。
/// @param up 
/// @param low 
/// @return 
rgba additive_blending(rgba& up, rgba& low);

/// @brief 乘法混合, 前景和背景的颜色值相乘，用于产生阴影和深度效果，融合后的图像通常更暗。
/// @param up 
/// @param low 
/// @return 
rgba multiplicative_blending(rgba& up, rgba& low);

/// @brief 屏幕混合, 前景和背景的反转颜色值相乘，再反转回来，用于产生较亮的效果，通常用于去除黑色背景。
/// @param up 
/// @param low 
/// @return 
rgba screen_blending(rgba& up, rgba& low);

int image_overlay(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    std::string upper_imgpath = args->get<std::string>("upper_imgpath");
    std::string lower_imgpath = args->get<std::string>("lower_imgpath");
    std::string dst_imgpath = args->get<std::string>("overlay_imgpath");

    double up_opacity = args->get<double>("upper_opacity");
    double low_opacity = args->get<double>("lower_opacity");

    std::string method = args->get<std::string>("method");

    PRINT_LOGGER(logger, info, fmt::format("upper_imgpath: \"{}\"",upper_imgpath));
    PRINT_LOGGER(logger, info, fmt::format("lower_imgpath: \"{}\"",lower_imgpath));
    PRINT_LOGGER(logger, info, fmt::format("up_opacity: {}",up_opacity));
    PRINT_LOGGER(logger, info, fmt::format("low_opacity: {}",low_opacity));
    PRINT_LOGGER(logger, info, fmt::format("method: \"{}\"",method));

    std::function<rgba(rgba&, rgba&)> overlay_func;

    if(method == "normal"){
        overlay_func = normal_alpha_blending;
    }
    else if(method == "premultiple"){
        overlay_func = premultiplied_alpha_blending;
    }
    else if(method == "mask"){
        overlay_func = mask_blending;
    }
    else if(method == "additive"){
        overlay_func = additive_blending;
    }
    else if(method == "multiple"){
        overlay_func = multiplicative_blending;
    }
    else if(method == "screen"){
        overlay_func = screen_blending;
    }
    else{
        PRINT_LOGGER(logger, error,"unsupported overlay method.");
        return -1;
    }

    auto ds_up = (GDALDataset*)GDALOpen(upper_imgpath.c_str(), GA_ReadOnly);
    if(!ds_up){
        PRINT_LOGGER(logger, error,"ds_up is nullptr");
        return -2;
    }
    int up_width = ds_up->GetRasterXSize();
    int up_height = ds_up->GetRasterYSize();
    GDALDataType up_datatype = ds_up->GetRasterBand(1)->GetRasterDataType();

    auto ds_low = (GDALDataset*)GDALOpen(lower_imgpath.c_str(), GA_ReadOnly);
    if(!ds_low){
        GDALClose(ds_up);
        PRINT_LOGGER(logger, error,"ds_low is nullptr");
        return -2;
    }
    int low_width = ds_low->GetRasterXSize();
    int low_height = ds_low->GetRasterYSize();
    GDALDataType low_datatype = ds_low->GetRasterBand(1)->GetRasterDataType();

    if(low_width != up_width || up_height != low_height){
        GDALClose(ds_up);GDALClose(ds_low);
        PRINT_LOGGER(logger, error,"size of ds_low is diff with ds_up");
        return -3;
    }
    fmt::print("height:{},width{}\n",up_height, up_width);

    if(low_datatype != up_datatype || up_datatype != GDT_Byte){
        GDALClose(ds_up);GDALClose(ds_low);
        PRINT_LOGGER(logger, error,"datatype of ds_low is diff with ds_up, or someone is not gdt_byte");
        return -3;
    }

    /// mem dst
    GDALDriver* dri_mem = GetGDALDriverManager()->GetDriverByName("MEM");
    GDALDataset* ds_dst = dri_mem->Create("", up_width, up_height, 4, GDT_Byte, nullptr);
    if(!ds_dst){
        GDALClose(ds_up);GDALClose(ds_low);
        PRINT_LOGGER(logger, error,"ds_dst(mem) is nullptr");
        return -3;
    }

    unsigned char* arr_up_r = new unsigned char[up_width];
    unsigned char* arr_up_g = new unsigned char[up_width];
    unsigned char* arr_up_b = new unsigned char[up_width];
    unsigned char* arr_up_a = new unsigned char[up_width];

    unsigned char* arr_low_r = new unsigned char[up_width];
    unsigned char* arr_low_g = new unsigned char[up_width];
    unsigned char* arr_low_b = new unsigned char[up_width];
    unsigned char* arr_low_a = new unsigned char[up_width];

    unsigned char* arr_dst_r = new unsigned char[up_width];
    unsigned char* arr_dst_g = new unsigned char[up_width];
    unsigned char* arr_dst_b = new unsigned char[up_width];
    unsigned char* arr_dst_a = new unsigned char[up_width];

    struct tmp_rb_arr
    {
        tmp_rb_arr(){}
        tmp_rb_arr(GDALRasterBand* rb_, unsigned char* arr_):rb(rb_),arr(arr_){}
        GDALRasterBand* rb;
        unsigned char* arr;
    };

    std::map<int, unsigned char*> arr_up_map = {
        {1, arr_up_r}, {2, arr_up_g}, {3, arr_up_b}, {4, arr_up_a}
    };
    std::map<int, unsigned char*> arr_low_map = {
        {1, arr_low_r}, {2, arr_low_g}, {3, arr_low_b}, {4, arr_low_a}
    };
    std::map<int, unsigned char*> arr_dst_map = {
        {1, arr_dst_r}, {2, arr_dst_g}, {3, arr_dst_b}, {4, arr_dst_a}
    };


    for(int row = 0; row < up_height; row++)
    {
        cout<<"\r"<<row;
        for(auto it : arr_up_map)
            ds_up->GetRasterBand(it.first)->RasterIO(GF_Read, 0, row, up_width, 1, it.second, up_width, 1, GDT_Byte, 0, 0);
        for(auto it : arr_low_map)
            ds_low->GetRasterBand(it.first)->RasterIO(GF_Read, 0, row, up_width, 1, it.second, up_width, 1, GDT_Byte, 0, 0);

#pragma omp parallel for
        for(int col = 0; col < up_width; col++)
        {
            int up_r = int(arr_up_r[col]), up_g = int(arr_up_g[col]), up_b = int(arr_up_b[col]), up_a = int(arr_up_a[col]);
            int low_r = int(arr_low_r[col]), low_g = int(arr_low_g[col]), low_b = int(arr_low_b[col]), low_a = int(arr_low_a[col]);

            up_a = int(up_a == 0 ? 0 : (up_opacity * 255));
            low_a = int(low_a == 0 ? 0 : (low_opacity * 255));

            rgba ori_up(up_r, up_g, up_b, up_a);
            rgba ori_low(low_r, low_g, low_b, low_a);
            rgba dst = overlay_func(ori_up, ori_low);

            arr_dst_r[col] = dst.red;
            arr_dst_g[col] = dst.green;
            arr_dst_b[col] = dst.blue;
            arr_dst_a[col] = dst.alpha;

        }
        // rasterio gf_write
        for(auto it : arr_dst_map)
            ds_dst->GetRasterBand(it.first)->RasterIO(GF_Write, 0, row, up_width, 1, it.second, up_width, 1, GDT_Byte, 0, 0);
    }
    cout<<endl;

    GDALDriver* dri_png = GetGDALDriverManager()->GetDriverByName("PNG");
    GDALDataset* ds_dst_png = dri_png->CreateCopy(dst_imgpath.c_str(), ds_dst, false, nullptr, nullptr, nullptr);
    if(!ds_dst_png){
        GDALClose(ds_up);GDALClose(ds_low);
        PRINT_LOGGER(logger, error,"ds_dst(png) is nullptr");
        return -3;
    }
    GDALClose(ds_dst_png);

    for(auto ptr : {arr_up_r, arr_up_g, arr_up_b, arr_up_a, arr_low_r, arr_low_g, arr_low_b, arr_low_a, arr_dst_r, arr_dst_g, arr_dst_b, arr_dst_a}){
        delete[] ptr;
    }
    
    PRINT_LOGGER(logger, info,"image_overlay success.");
    return 1;
}





rgba normal_alpha_blending(rgba& up, rgba& low)
{
    double up_opacity = up.alpha / 255.;
    double low_opacity = low.alpha / 255.;
    
    double opacity_out = up_opacity + (1 - up_opacity) * low_opacity;
    int red_out = (up.red * up_opacity + low.red * low_opacity * (1 - up_opacity)) / opacity_out;
    int green_out = (up.green * up_opacity + low.green  * low_opacity * (1 - up_opacity)) / opacity_out;
    int blue_out = (up.blue * up_opacity + low.blue  * low_opacity * (1 - up_opacity)) / opacity_out;

    return rgba(red_out, green_out, blue_out, int(opacity_out * 255));
}

rgba premultiplied_alpha_blending(rgba& up, rgba& low)
{
    double up_opacity = up.alpha / 255;
    double low_opacity = low.alpha / 255;
    
    int red_out = up.red + low.red * (1 - up_opacity);
    int green_out = up.green + low.green * (1 - up_opacity);
    int blue_out = up.blue + low.blue * (1 - up_opacity);
    float opacity_out = up_opacity + low_opacity * (1 - up_opacity);

    return rgba(red_out, green_out, blue_out, int(opacity_out * 255));
}

rgba mask_blending(rgba& up, rgba& low)
{
    double up_opacity = up.alpha / 255.;
    double low_opacity = low.alpha / 255.;

    float alphaM = 0.5;

    int red_out = up.red * alphaM + low.red * (1 - alphaM);
    int green_out = up.green * alphaM + low.green * (1 - alphaM);
    int blue_out = up.blue * alphaM + low.blue * (1 - alphaM);
    int opacity_out = alphaM * up_opacity + (1 - alphaM) * low_opacity;

    return rgba(red_out, green_out, blue_out, opacity_out);
}

rgba additive_blending(rgba& up, rgba& low)
{
    int red_out = std::min(255, up.red + low.red);
    int green_out = std::min(255, up.green + low.green);
    int blue_out = std::min(255, up.blue + low.blue);
    int opacity_out = std::min(255, up.alpha + low.alpha);

    return rgba(red_out, green_out, blue_out, opacity_out);
}


rgba multiplicative_blending(rgba& up, rgba& low)
{
    double up_opacity = up.alpha / 255.;
    double low_opacity = low.alpha / 255.;

    int red_out = up.red * low.red / 255.0;
    int green_out = up.green * low.green / 255.0;
    int blue_out = up.blue * low.blue / 255.0;
    float opacity_out = up_opacity * low_opacity;

    return rgba(red_out, green_out, blue_out, int(opacity_out * 255));
}


rgba screen_blending(rgba& up, rgba& low)
{
    double up_opacity = up.alpha / 255.;
    double low_opacity = low.alpha / 255.;


    int red_out = 255 - ((255 - up.red) * (255 - low.red) / 255);
    int green_out = 255 - ((255 - up.green) * (255 - low.green) / 255);
    int blue_out = 255 - ((255 - up.blue) * (255 - low.blue) / 255);
    float opacity_out = up_opacity + low_opacity - up_opacity * low_opacity;

    return rgba(red_out, green_out, blue_out, int(opacity_out * 255));
}
