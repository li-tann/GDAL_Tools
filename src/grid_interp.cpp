#include "raster_include.h"
#include <gdal_alg.h>

int grid_interp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string points_path  = args->get<string>("points_path");
	double spacing  = args->get<double>("spacing");
    std::string raster_path  = args->get<string>("raster_path");

    std::ifstream ifs(points_path);
    if(!ifs.is_open()){
        PRINT_LOGGER(logger, error, "ifs open failed.");
		return -1;
    }

    std::string str_temp;
    std::vector<std::string> str_list;
    std::vector<xyz> points;
    while(std::getline(ifs, str_temp))
    {
        strSplit(str_temp, str_list, ",");
        if(str_list.size() >= 3){
            points.push_back(xyz(std::stod(str_list[0]), std::stod(str_list[1]), std::stod(str_list[2])));
        }
    }
    ifs.close();

    if(points.size() < 0){
        PRINT_LOGGER(logger, error, "there is no valid data in points_path.");
        return -2;
    }

    int points_num = points.size();
    double* arr_x = new double[points_num];
    double* arr_y = new double[points_num];
    double* arr_z = new double[points_num];

    double max_x = points[0].x;
    double min_x = points[0].x;
    double max_y = points[0].y;
    double min_y = points[0].y;

    for(int i=0; i<points_num; i++){
        arr_x[i] = points[i].x;
        arr_y[i] = points[i].y;
        arr_z[i] = points[i].z;

        if(arr_x[i] > max_x) max_x = arr_x[i];
        if(arr_x[i] < min_x) min_x = arr_x[i];
        if(arr_y[i] > max_y) max_y = arr_y[i];
        if(arr_y[i] < min_y) min_y = arr_y[i];
    }
    points.clear();

    max_x = floor(max_x);
    max_y = floor(max_y);
    min_x = ceil(min_x);
    min_y = ceil(min_y);

    int height = (max_y - min_y) / spacing;
    int width  = (max_x - min_x) / spacing;

    float* arr_out = new float[height * width];


    GDALAllRegister();
	GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
	GDALDataset* ds_out = driver->Create(raster_path.c_str(), width, height, 1, GDT_Float32, nullptr);
	if(!ds_out){
		delete[] arr_x, arr_y, arr_z, arr_out;
		PRINT_LOGGER(logger, error, "ds_out is nullptr.");
        return -3;
	}


    /// radius, 需要根据实际的DEM尺寸与SAR尺寸做对比
	GDALGridInverseDistanceToAPowerOptions options;
	options.dfPower = 2;
	options.dfRadius1 = 20;
	options.dfRadius2 = 15;
	options.dfNoDataValue = NAN;

	int current_percentage = -1;
	auto grid_process = [&](double dfComplete, const char *pszMessage, void *pProgressArg){
		// log_msg(info_type::info, fmt::format("lookuptable::draw_lookuptable_sar_grid, GDALGridCreate.progress: {}",dfComplete));
		if(int(dfComplete * 100) > current_percentage){
			current_percentage = int(dfComplete * 100);
			std::cout<<"\r lookuptable::draw_lookuptable_sar_grid, GDALGridCreate.progress:"<<current_percentage<<"%";
		}
		return true;
	};
	
	/// lon在SAR坐标系下的分布
	CPLErr err;
	err = GDALGridCreate(GDALGridAlgorithm::GGA_InverseDistanceToAPower, &options, 
						points_num, arr_x, arr_y, arr_z,
						min_x, max_x, min_y, max_y, width, height, GDT_Float32, arr_out, (GDALProgressFunc&)grid_process, NULL);

    delete[] arr_x, arr_y, arr_z;
	if(err != CE_None){
		delete[] arr_out;
		GDALClose(ds_out);
		PRINT_LOGGER(logger, error, "GDALGridCreate return err.");
        return -4;
	}

    ds_out->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, width, height, arr_out, width, height, GDT_Float32, 0, 0);
    delete[] arr_out;
    GDALClose(ds_out);

    PRINT_LOGGER(logger, info, "grid_interp finished.");
    return 1;
}