#include "raster_include.h"

/*
    sub_band_extract.add_argument("input")
        .help("raster image (dem), with short or float datatype.");

    sub_band_extract.add_argument("val_thres")
        .help("value threshold, compare with the abs diff between the cur_point's value and mean of surrounding points's value.")
        .scan<'g',double>();

    // sub_image_overlay.add_argument("output_txt")
    //     .help("txt output filepath, like: pos0.y, pos0.x\\n pos1.y, pos1.x\\n... ");

    sub_points_extract.add_argument("output_mask_tif")
            .help("mask.tif with byte datatype, has the same coord with input tif.");
    
    sub_image_overlay.add_argument("output_type")
        .help("the unit of output points, like pixel or degree (same with geotransform's unit).")
        .choices("pixel","geo");
*/

#define DEM_DEGREE_TO_METER (30.0 / 0.000277777777778)

#define PRINT_TXT false

int very_important_points(float* arr, char* mask, int height, int width, double spacing, double thres);

int import_points_extract(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("input");
	double thres  = args->get<double>("val_thres");
    std::string output_type  = args->get<string>("output_type");
#if PRINT_TXT
    std::string output_txtpath  = args->get<string>("output_txt");
#endif
    std::string mask_path  = args->get<string>("output_mask_tif");


    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto dataset = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly);
    if(!dataset){
        PRINT_LOGGER(logger, error, "dataset is nullptr.");
        return -1;
    }

    int width = dataset->GetRasterXSize();
    int height = dataset->GetRasterYSize();

    auto band = dataset->GetRasterBand(1);
    auto datatype = band->GetRasterDataType();

    if(datatype != GDT_Float32 && datatype != GDT_Int16){
        PRINT_LOGGER(logger, error, "datatype is not both float and short.");
        GDALClose(dataset);
        return -1;
    }

    /// @note 六参数
    double gt[6];
    if(output_type == "geo"){
        dataset->GetGeoTransform(gt);
    }
    else{
        /// pixel
        gt[0] = gt[3] = 0;
        gt[1] = gt[5] = 1;
    }
    /// @note 坐标系统 
    auto projection = dataset->GetProjectionRef();

    /// @note DEM数组
    float* arr = new float[width * height];

    switch (datatype)
    {
    case GDT_Int16:{
        short* arr_tmp = new short[width];
        for(int i=0; i<height; i++){
            band->RasterIO(GF_Read, 0, i, width, 1, arr_tmp, width, 1, datatype, 0, 0);
            for(int j=0; j<width; j++){
                arr[i * width + j] = arr_tmp[j];
            }
        }
        delete[] arr_tmp;
    }break;
    case GDT_Float32:{
        band->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, datatype, 0, 0);
    }break;
    /// no need about default
    }
    GDALClose(dataset);

    /// pixel to geo
    auto get_pos = [&](double x, double y){
        double pos_x = gt[0] + x * gt[1];
        double pos_y = gt[3] + y * gt[5];
        return xyz(pos_x, pos_y, 0);
    };

    /// geo to pixel
    auto anti_pos = [&](double x, double y){
        double pos_x = (x - gt[0]) / gt[1];
        double pos_y = (y - gt[3]) / gt[5];
        return xyz(pos_x, pos_y, 0);
    };

    /// 预筛选
    // vector<xyz> important_points;
    char* mask = new char[height * width];

    int vip_num = very_important_points(arr, mask, height, width, DEM_DEGREE_TO_METER*gt[1], thres);
    std::cout<<fmt::format("number of very important points: {}/{}", vip_num, height*width)<<std::endl;

#if 0
    /// second filter
    vector<xyz> filtered_points;
    char* filtered_mask = new char[height * width];
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            int idx = i * width + j;
            filtered_mask[idx] = 0;
            float current = arr[idx];
            auto pt = get_pos(j,i);
            pt.z = current;

            if(mask[idx] == 0)
                continue;

            if(i==0 || j==0 || i==height-1 || j==width-1){
                
                filtered_points.push_back(pt);
                filtered_mask[idx] = 1;
                continue;
            }

            char upleft = mask[idx - width - 1];
            char up     = mask[idx - width];
            char upright = mask[idx - width + 1];
            char left = mask[idx - 1];
            char right = mask[idx + 1];
            char downleft = mask[idx + width - 1];
            char down     = mask[idx + width];
            char downright = mask[idx + width + 1];
            if(upleft + downright == 2 || up + down == 2 || upright + downleft == 2 || left + right == 2){
                /// is not important points
                continue;
            }else{
                filtered_points.push_back(pt);
                filtered_mask[idx] = 1;
            }
        }
    }
    delete[] arr;
    delete[] mask;
    mask = filtered_mask;
    important_points.clear();
    important_points = filtered_points;
    std::cout<<fmt::format("filterd points.size: {}\n",filtered_points.size());
#endif

    /// write mask
    do{
        GDALDriver* dri_tiff = GetGDALDriverManager()->GetDriverByName("GTiff");
        auto ds_out = dri_tiff->Create(mask_path.c_str(), width, height, 1, GDT_Byte, NULL);
        if(!ds_out){
            PRINT_LOGGER(logger, error, "ds_out(mask) is nullptr.");
            break;
        }
        auto bd_out = ds_out->GetRasterBand(1);
        bd_out->RasterIO(GF_Write, 0, 0, width, height, mask, width, height, GDT_Byte, 0, 0);
        if(output_type == "geo"){
            ds_out->SetGeoTransform(gt);
            ds_out->SetProjection(projection);
        }
        GDALClose(ds_out);
    }while(false);

#if PRINT_TXT
    std::ofstream ofs(output_txtpath);
    if(!ofs.is_open()){
        PRINT_LOGGER(logger, error, "datatype is not both float and short.");
        return -2;
    }

    for(const auto& pt : important_points)
    {
        ofs<<fmt::format("{},{},{}\n", pt.y, pt.x, pt.z);
    }
    ofs.close();
#endif

    PRINT_LOGGER(logger, info, "import_points_extract finished.");
    return 1;
}

/// @brief 提取DEM中的VIP点
/// @param arr DEM数组
/// @param mask 掩膜数组, 尺寸与DEM数组相同, 若DEM[i]点为VIP点，则mask[i]标记为1
/// @param height DEM高
/// @param width DEM宽
/// @param spacing 分辨率/m
/// @param thres 阈值, 当高差超过阈值后判定为VIP点
/// @return 返回vip点数量
int very_important_points(float* arr, char* mask, int height, int width, double spacing, double thres)
{   
    /// 计算center到直线h1h2的垂直距离, spacing分辨率(m)
    auto get_vertical_range = [](float spacing, float h1, float h2, float h_cen){
        float h_mean = (h1+h2)/2;
        float delta_h = abs(h_cen - h_mean);
        double cos_theta = spacing*2 / sqrt(pow(h1-h2,2)+pow(spacing*2,2));
        return delta_h * cos_theta;
    };

    int vip_num = 0;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            int idx = i*width+j;
            float current = arr[idx];

            /// @note 四个角点肯定是VIP点
            if((i == 0 || i == height-1) && (j == 0 || j == width-1)){
                mask[idx] = 1;
                vip_num++;
                continue;
            }

            mask[idx] = 0;
            double diff = 0;
            if( i == 0 || i == height - 1){
                /// top or down
                float left = arr[idx - 1];
                float right = arr[idx + 1];
                diff = get_vertical_range(spacing, left, right, current);
            }
            else if(j == 0 || j == width - 1){
                /// left or right
                float up = arr[idx - width];
                float down = arr[idx + width];
                diff = get_vertical_range(spacing, up, down, current);
            }
            else{
                float upleft = arr[idx - width - 1];
                float up     = arr[idx - width];
                float upright = arr[idx - width + 1];
                float left = arr[idx - 1];
                float right = arr[idx + 1];
                float downleft = arr[idx + width - 1];
                float down     = arr[idx + width];
                float downright = arr[idx + width + 1];
                diff = get_vertical_range(spacing, up, down, current)
                       + get_vertical_range(spacing, left, right, current)
                       + get_vertical_range(spacing*sqrt(2), upleft, downright, current)
                       + get_vertical_range(spacing*sqrt(2), upright, downleft, current);
                diff /= 4;
            }
            if(diff >= thres){
                mask[idx] = 1;
                vip_num++;
            }

        } /// j
    } /// i

    return vip_num;
}