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

int points_shapefile_dilution_v2(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_in = args->get<string>("point_shapefile");
    string shp_out = args->get<string>("diluted_shapefile");
    int new_width  = args->get<int>("new_width");
    int new_height = args->get<int>("new_height");

    std::cout<<fmt::format("shapefile  in: '{}'", shp_in)<<std::endl;
    std::cout<<fmt::format("shapefile out: '{}'", shp_out)<<std::endl;
    std::cout<<fmt::format("new width : '{}'", new_width)<<std::endl;
    std::cout<<fmt::format("new height: '{}'", new_height)<<std::endl;

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    /// @note 检查, 如果shp输入形式不对, 直接报错退出
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

    /// @note step.1基于shp, 计算四至范围, 根据new_width, new_height 计算步长, 并生成一个lambda函数, 输入之前的坐标, 可以计算出新的坐标(整数)
    int origin_feature_num = layer->GetFeatureCount();
    std::cout<<fmt::format("layer->GetFeatureCount(): {}", origin_feature_num)<<std::endl;

    OGRFeature* feature;
    int feature_idx = -1;
    double x_max, x_min, y_max, y_min;
    bool point_init = false;
    while ((feature = layer->GetNextFeature()))
    {
        ++feature_idx;

        OGRGeometry* poGeometry = feature->GetGeometryRef();
        if (poGeometry == nullptr || wkbFlatten(poGeometry->getGeometryType()) != wkbPoint){
            continue;
        }

        OGRPoint* point = (OGRPoint*)poGeometry;
        double tmp_x = point->getX();
        double tmp_y = point->getY();
        if(!point_init){
            x_max = tmp_x;
            x_min = tmp_x;
            y_max = tmp_y;
            y_min = tmp_y;
            point_init = true;
        }
        else{
            x_max = x_max > tmp_x ? x_max : tmp_x;
            x_min = x_min < tmp_x ? x_min : tmp_x;
            y_max = y_max > tmp_y ? y_max : tmp_y;
            y_min = y_min < tmp_y ? y_min : tmp_y;
        }

        // 释放Feature
        OGRFeature::DestroyFeature(feature);
    }
    double x_step = (x_max - x_min) / (new_width-1);
    double y_step = (y_max - y_min) / (new_height-1);

    std::cout<<fmt::format("1st loop feature, loop count: {}", feature_idx+1)<<std::endl;

    std::cout<<fmt::format("x range: [{},{}] ", x_min, x_max)<<std::endl;
    std::cout<<fmt::format("y range: [{},{}] ", y_min, y_max)<<std::endl;
    std::cout<<fmt::format("x.step: {}, y.step:{}", x_step, y_step)<<std::endl;

    auto old_pos_2_new_pos_idx = [&](double x, double y)->int{
        int col = int((x - x_min) / x_step);
        int row = int((y - y_min) / y_step);
        // if(col >= new_width)col = new_width-1;
        // if(row >= new_height)row = new_height-1;
        return row * new_width + col;
    };

    /// @note step.2 遍历所有点, 创建map<new_point, vector<old_point>>
    std::map<int, std::vector<int>> new_point_idx_2_old_points_idx;
    layer->ResetReading();
    feature_idx = -1;
    while ((feature = layer->GetNextFeature()))
    {
        ++feature_idx;

        OGRGeometry* poGeometry = feature->GetGeometryRef();
        if (poGeometry == nullptr || wkbFlatten(poGeometry->getGeometryType()) != wkbPoint){
            continue;
        }

        OGRPoint* point = (OGRPoint*)poGeometry;
        double tmp_x = point->getX();
        double tmp_y = point->getY();
        
        int new_idx = old_pos_2_new_pos_idx(tmp_x, tmp_y);
        if(new_point_idx_2_old_points_idx.find(new_idx) == new_point_idx_2_old_points_idx.end()){
            new_point_idx_2_old_points_idx[new_idx] = std::vector<int>();
        }

        /// @note 新点到旧点的映射关系
        new_point_idx_2_old_points_idx[new_idx].push_back(feature_idx);

        // 释放Feature
        OGRFeature::DestroyFeature(feature);
    }
    std::cout<<fmt::format("new_point_idx_2_old_points_idx.size: {}", new_point_idx_2_old_points_idx.size())<<std::endl;
    std::cout<<fmt::format("2nd loop feature, loop count: {}", feature_idx+1)<<std::endl;


    /// @note step.3 获取shp中float格式的feature数量和名称, 方便为新的shp创建feature (仅获取float类型, 求均值无误差)

    /// @note 输入的shp文件中, 所有float格式数据的索引+名称
    std::map<int, std::string> feature_defn_map;

    OGRFeatureDefn* defn = layer->GetLayerDefn();
    for (int i = 0; i < defn->GetFieldCount(); ++i) {
        OGRFieldDefn* fieldDefn = defn->GetFieldDefn(i);
        std::string name = fieldDefn->GetNameRef();
        if(fieldDefn->GetType() == OGRFieldType::OFTReal){
            feature_defn_map.insert(std::pair<int, std::string>(i, name));
        }
    }

    if(feature_defn_map.size() < 1){
        PRINT_LOGGER(logger, error, fmt::format("there is no valid feature defn in this shp file."));
        GDALClose(ds);
        return -1;
    }

    /// @note step.4 遍历map, 获取new_point映射的所有old_points, 然后用feature->GetRawFieldRef(old_point_idx)->Real来获取所有老点的坐标, 然后计算均值, 得到新点的val
    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
    GDALDataset* ds_out = driver->Create(shp_out.c_str(), 0, 0, 0, GDT_Unknown, NULL);
    if(!ds_out){
        GDALClose(ds);
        PRINT_LOGGER(logger, error, fmt::format("points_shapefile_dilution failed, ds_out is nullptr."));
        return -2;
    }

    OGRLayer* layer_out = ds_out->CreateLayer("points", layer->GetSpatialRef(), wkbPoint, NULL);


    OGRFieldDefn field_defn_val("ID", OFTInteger);
    layer_out->CreateField(&field_defn_val);

    for(auto& it : feature_defn_map){
        OGRFieldDefn field_defn_val(it.second.c_str(), OFTReal);
        layer_out->CreateField(&field_defn_val);
    }
    
    OGRPoint point;
    for(auto& it : new_point_idx_2_old_points_idx)
    {
        int id = it.first;

        /// 遍历所有相关的旧点, 从中提取出点
        std::vector<float> defn_val_sum(feature_defn_map.size(), 0);
        std::vector<int> defn_val_count(feature_defn_map.size(), 0);
        for(auto& old_idx : it.second)
        {
            OGRFeature* tmp_feature = layer->GetFeature(old_idx);
            if(!tmp_feature){
                continue;
            }
            int defn_idx = 0;
            for(auto& defn : feature_defn_map)
            {
                auto v = tmp_feature->GetFieldAsDouble(defn.first);
                if(isnan(v)){
                    continue;
                }
                defn_val_sum[defn_idx] += v;
                defn_val_count[defn_idx]++;
                
                ++defn_idx;
            }
            OGRFeature::DestroyFeature(tmp_feature);            
        }
        for(int i=0; i<defn_val_sum.size(); i++){
            defn_val_sum[i] = defn_val_count[i] == 0 ? NAN : defn_val_sum[i] / defn_val_count[i];
        }

        OGRFeature* tmp_feature = OGRFeature::CreateFeature(layer_out->GetLayerDefn());

        int new_row = id / new_width;
        int new_col = id  - new_row * new_width;

        point.setX(x_min + (new_col+0.5) * x_step);
        point.setY(y_min + (new_row+0.5) * y_step);

        tmp_feature->SetGeometry(&point);
        int idx = 0;
        for(auto& it : feature_defn_map){
            // tmp_feature->SetField(target_defn.c_str(), val);
            std::string defn_str = it.second;
            double val = defn_val_sum[idx];
            tmp_feature->SetField(defn_str.c_str(), val);
            idx++;
        }
        tmp_feature->SetField("ID", id);

        layer_out->CreateFeature(tmp_feature);
        OGRFeature::DestroyFeature(tmp_feature);
    }

    GDALClose(ds);
    GDALClose(ds_out);

    PRINT_LOGGER(logger, info, "points_shapefile_dilution finished.");
    return 1;
}