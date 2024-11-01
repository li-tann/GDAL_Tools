#include "raster_include.h"

/*
    sub_band_extract.add_argument("input")
        .help("raster image (dem), with short or float datatype.");

    sub_band_extract.add_argument("val_thres")
        .help("value threshold, compare with the abs diff between the cur_point's value and mean of surrounding points's value.")
        .scan<'g',double>();

    sub_image_overlay.add_argument("output_txt")
        .help("txt output filepath, like: pos0.y, pos0.x\\n pos1.y, pos1.x\\n... ");

    sub_points_extract.add_argument("output_mask_tif")
            .help("mask.tif with byte datatype, has the same coord with input tif.");
    
    sub_image_overlay.add_argument("output_type")
        .help("the unit of output points, like pixel or degree (same with geotransform's unit).")
        .choices("pixel","geo");
*/

int import_points_extract(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("input");
	double thres  = args->get<double>("val_thres");
    std::string output_type  = args->get<string>("output_type");
    std::string output_txtpath  = args->get<string>("output_txt");
    std::string mask_path  = args->get<string>("output_mask_tif");


    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto dataset = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly());
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

    double gt[6];
    if(output_type == "geo"){
        dataset->GetGeoTransform(gt);
    }
    else{
        /// pixel
        gt[0] = gt[3] = 0;
        gt[1] = gt[5] = 1;
    }

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
    vector<xyz> important_points;
    char* mask = new char[height * width];
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            int idx = i * width + j;
            mask[idx] = 0;
            float current = arr[idx];
            auto pt = get_pos(j,i);
            pt.z = current;
            if((i == 0 && j == 0) || (i == 0 && j == width - 1) || (i == height - 1 && j == 0) || (i == height - 1 && j == width - 1)){
                /// corner
                important_points.push_back(pt);
                mask[idx] = 1;
                continue;
            }

            double diff = 0;
            if( i == 0 || i == height - 1){
                /// top or down
                float left = arr[idx - 1];
                float right = arr[idx + 1];
                diff = abs(current - (left + right)/2);
            }
            else if(j == 0 || j == width - 1){
                /// left or right
                float up = arr[idx - width];
                float down = arr[idx + width];
                diff = abs(current - (up + down)/2);
            }
            else {
                float upleft = arr[idx - width - 1];
                float up     = arr[idx - width];
                float upright = arr[idx - width + 1];
                float left = arr[idx - 1];
                float right = arr[idx + 1];
                float downleft = arr[idx + width - 1];
                float down     = arr[idx + width];
                float downright = arr[idx + width + 1];
                diff = abs(current - (upleft + up + upright + left + right + downleft + down + downright)/8);
            }

            if(diff >= thres){
                important_points.push_back(pt);
                mask[idx] = 1;
            }
        }
    }
    std::cout<<fmt::format("very important points.size: {}\n",important_points.size());

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
            ds_out->SetProjection(dataset->GetProjectionRef());
        }
        GDALClose(ds_out);
    }while(false);

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

    PRINT_LOGGER(logger, info, "import_points_extract finished.");
    return 1;
}