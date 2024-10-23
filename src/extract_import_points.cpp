#include "raster_include.h"

/*
    sub_band_extract.add_argument("input")
        .help("raster image (dem), with short or float datatype.");

    sub_band_extract.add_argument("val_thres")
        .help("value threshold, compare with the abs diff between the cur_point's value and mean of surrounding points's value.")
        .scan<'g',double>();

    sub_image_overlay.add_argument("output_type")
        .help("the unit of output points, like pixel or degree (same with geotransform's unit).")
        .choices("pixel","geo");

    sub_image_overlay.add_argument("output_txt")
        .help("txt output filepath, like: pos0.y, pos0.x\\n pos1.y, pos1.x\\n... ");
*/

int import_points_extract(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("input");
	double thres  = args->get<double>("val_thres");
    std::string output_type  = args->get<string>("output_type");
    std::string output_path  = args->get<string>("output_txt");


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

    vector<xyz> important_points;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            int idx = i * width + j;
            float current = arr[idx];
            auto pt = get_pos(j,i);
            if((i == 0 && j == 0) || (i == 0 && j == width - 1) || (i == height - 1 && j == 0) || (i == height - 1 && j == width - 1)){
                /// corner
                pt.z = current;
                important_points.push_back(pt);
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

            // if(i == j && i %15 == 0){
            //     std::cout<<fmt::format("i:{},j:{},diff:{},current:{}",i,j,diff, current)<<std::endl;
            // }

            if(diff >= thres){
                pt.z = current;
                important_points.push_back(pt);
            }
        }
    }
    delete[] arr;
    std::cout<<fmt::format("important points.size: {}\n",important_points.size());

    std::ofstream ofs(output_path);
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