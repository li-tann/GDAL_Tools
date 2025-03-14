#include "vector_include.h"

/*
    sub_point_shp_dilution.add_argument("point_shapefile")
        .help("input points shapefile.");

    sub_point_shp_dilution.add_argument("ref_dem")
        .help("reference DEM.");

    sub_point_shp_dilution.add_argument("target_defn")
            .help("target FieldDefn.");    

    sub_point_shp_dilution.add_argument("diluted_shapefile")
        .help("diluted point shapefile.");
*/
int points_shapefile_dilution(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_in = args->get<string>("point_shapefile");
    string ref_dem = args->get<string>("ref_dem");
    string target_defn = args->get<string>("target_defn");
    string shp_out = args->get<string>("diluted_shapefile");

    std::cout<<fmt::format("shapefile  in: '{}'", shp_in)<<std::endl;
    std::cout<<fmt::format("shapefile out: '{}'", shp_out)<<std::endl;
    std::cout<<fmt::format("reference dem: '{}'", ref_dem)<<std::endl;
    std::cout<<fmt::format(" target  defn: '{}'", target_defn)<<std::endl;

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    /// @note 加载DEM基本信息
    int dem_h, dem_w;
    double gt[6];
    {
        GDALDataset* ds_dem = (GDALDataset*)GDALOpen(ref_dem.c_str(), GA_ReadOnly);
        if(!ds_dem){
            PRINT_LOGGER(logger, error, "points_shapefile_dilution failed, ds_dem is nullptr.");
            return -1;
        }
        dem_h = ds_dem->GetRasterYSize();
        dem_w = ds_dem->GetRasterXSize();
        ds_dem->GetGeoTransform(gt);
        GDALClose(ds_dem);
    }

    /// @note 检查输入shp的基本信息
    GDALDataset* ds = (GDALDataset*)GDALOpenEx(shp_in.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
    if(!ds){
        PRINT_LOGGER(logger, error, "points_shapefile_dilution failed, ds is nullptr.");
        return -1;
    }

    OGRLayer* layer = ds->GetLayer(0);
    auto geomtype = layer->GetGeomType();
    std::cout<<fmt::format("geometry type: {}", OGRGeometryTypeToName(geomtype))<<std::endl;

    if(geomtype != decltype(geomtype)::wkbPoint){
        PRINT_LOGGER(logger, error, fmt::format("points_shapefile_dilution failed, geomtype isn't wkbPoint."));
        GDALClose(ds);
        return -1;
    }

    
    /// @note 搜索指定的Defn对应的索引值
    int target_defn_idx = -1;
    OGRFeatureDefn* defn = layer->GetLayerDefn();
    for (int i = 0; i < defn->GetFieldCount(); ++i) {
        OGRFieldDefn* fieldDefn = defn->GetFieldDefn(i);
        std::string name = fieldDefn->GetNameRef();
        if(name == target_defn && fieldDefn->GetType() == OGRFieldType::OFTReal){
            target_defn_idx = i;
            break;
        }
    }

    if(target_defn_idx < 0){
        PRINT_LOGGER(logger, error, fmt::format("points_shapefile_dilution failed, there is no valid defn in layer (no same with name or type(real))."));
        GDALClose(ds);
        return -1;
    }
    
    double* arr_val = new double[dem_h*dem_w];
    double* arr_num = new double[dem_h*dem_w];
    std::fill(arr_val, arr_val+dem_h*dem_w, 0);
    std::fill(arr_num, arr_num+dem_h*dem_w, 0);
    
    /// @note 更新arr_val, arr_num
    int origin_feature_num = layer->GetFeatureCount();
    OGRFeature* feature;
    while ((feature = layer->GetNextFeature()))
    {
        OGRGeometry* poGeometry = feature->GetGeometryRef();
        if (poGeometry == nullptr || wkbFlatten(poGeometry->getGeometryType()) != wkbPoint){
            continue;
        }

        OGRPoint* point = (OGRPoint*)poGeometry;
        double lon = point->getX();
        double lat = point->getY();

        int pix_x = int(0.5 + (lon - gt[0]) / gt[1]);
        int pix_y = int(0.5 + (lat - gt[3]) / gt[5]);

        if(pix_x < 0 || pix_x >= dem_w || pix_y < 0 || pix_y >= dem_h){
            continue;
        }

        int arr_idx = pix_y * dem_w + pix_x;
        double val = feature->GetFieldAsDouble(target_defn_idx);

        arr_val[arr_idx] += val;
        arr_num[arr_idx] ++;
        
        // 释放Feature
        OGRFeature::DestroyFeature(feature);
    }

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    GDALDataset* ds_out = driver->Create(shp_out.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if(!ds_out){
        GDALClose(ds);
        delete[] arr_val;
        delete[] arr_num;
        PRINT_LOGGER(logger, error, fmt::format("points_shapefile_dilution failed, ds_out is nullptr."));
        return -2;
    }

    OGRLayer* layer_out = ds_out->CreateLayer("points", layer->GetSpatialRef(), wkbPoint, NULL);
    /// @note 使用完shp_in的GetSpatialRef, 就可以删除了
    GDALClose(ds);

    OGRFieldDefn field_defn_val(target_defn.c_str(), OFTReal);
    field_defn_val.SetPrecision(3);
    layer_out->CreateField(&field_defn_val);

    OGRPoint point;
    int dilutioned_feature_num = 0;
    for(int i=0; i<dem_h; i++)
    {
        double lat = gt[3] + gt[5] * i;
        for(int j=0; j<dem_w; j++)
        {
            double lon = gt[0] + gt[1] * j;
            if(arr_num[i*dem_w+j]==0){
                continue;
            }
            double val = arr_val[i*dem_w+j] / arr_num[i*dem_w+j];

            point.setX(lon);
            point.setY(lat);

            OGRFeature* tmp_feature = OGRFeature::CreateFeature(layer_out->GetLayerDefn());
            tmp_feature->SetGeometry(&point);
            tmp_feature->SetField(target_defn.c_str(), val);

            layer_out->CreateFeature(tmp_feature);
            ++dilutioned_feature_num;

            OGRFeature::DestroyFeature(tmp_feature);
        }
    }

    std::cout<<fmt::format("  original feature count: {}", origin_feature_num)<<std::endl;
    std::cout<<fmt::format("dilutioned feature count: {}", dilutioned_feature_num)<<std::endl;

    GDALClose(ds_out);
    delete[] arr_val;
    delete[] arr_num;

    PRINT_LOGGER(logger, info, "points_shapefile_dilution finished.");
    return 1;
}