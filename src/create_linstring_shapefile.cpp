#include "vector_include.h"

int psnetwork_to_shapefile(const char* arc_file, const char* pspnt_file, const char* shp_file);

/*
    sub_create_linestring_shp.add_argument("points_file")
        .help("input points file with size of num_point*2(h*w), and format of int or double.");

    sub_create_linestring_shp.add_argument("arcs_file")
        .help("input arcs file with size of num_arc*2(h*w), and format of int.");

    sub_create_linestring_shp.add_argument("shp_file")
        .help("output linestring shapefile.");

    sub_create_linestring_shp.add_argument("-w", "--wgs84")
        .help("geo-coordination.")
        .flag();
*/

int create_linestring_shp(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string point_file = args->get<std::string>("points_file");
    std::string arc_file = args->get<std::string>("arcs_file");
    std::string output_shpfile = args->get<std::string>("shp_file");
    bool flag_wgs84 = args->is_used("--wgs84");

    double flag_geo = flag_wgs84 ? 1 : -1;

    PRINT_LOGGER(logger, info, fmt::format("point_file : [{}]", point_file));
    PRINT_LOGGER(logger, info, fmt::format("arc_file   : [{}]", arc_file));
    PRINT_LOGGER(logger, info, fmt::format("op_shpfile : [{}]", output_shpfile));
    PRINT_LOGGER(logger, info, fmt::format("flag_wgs84 : [{}]", flag_wgs84 ?"true": "false"));
    
    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_arc = (GDALDataset*)GDALOpen(arc_file.c_str(), GA_ReadOnly);
    if(!ds_arc){
        PRINT_LOGGER(logger, error, "ds_arc is nullptr.");
        return -1;
    }
    int arc_num = ds_arc->GetRasterYSize();
    int arc_width = ds_arc->GetRasterXSize();
    if(arc_width != 2){
        GDALClose(ds_arc);
        PRINT_LOGGER(logger, error, "arc_width != 2.");
        return -2;
    }
    GDALDataType arc_datatype = ds_arc->GetRasterBand(1)->GetRasterDataType();
    if(arc_datatype != GDT_Int32){
        GDALClose(ds_arc);
        PRINT_LOGGER(logger, error, "arc_datatype != GDT_Int32.");
        return -2;
    }

    GDALDataset* ds_pnts = (GDALDataset*)GDALOpen(point_file.c_str(), GA_ReadOnly);
    if(!ds_pnts){
        GDALClose(ds_arc);
        PRINT_LOGGER(logger, error, "ds_pnts is nullptr.");
        return -1;
    }
    int pnts_num = ds_pnts->GetRasterYSize();
    int pnts_width = ds_pnts->GetRasterXSize();
    if(pnts_width != 2){
        GDALClose(ds_arc);
        GDALClose(ds_pnts);
        PRINT_LOGGER(logger, error, "pnts_width != 2.");
        return -2;
    }
    GDALDataType pnts_datatype = ds_pnts->GetRasterBand(1)->GetRasterDataType();
    if(pnts_datatype != GDT_Int32 && pnts_datatype != GDT_Float64){
        GDALClose(ds_arc);
        GDALClose(ds_pnts);
        PRINT_LOGGER(logger, error, "pnts_datatype != GDT_Int32 && arc_datatype != GDT_Float64.");
        return -2;
    }

    int* arr_arc = new int[arc_num * 2];
    double* arr_pnts = new double[pnts_num * 2];
    ds_arc->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, 2, arc_num, arr_arc, 2, arc_num, GDT_Int32, 0, 0);
    if(pnts_datatype == GDT_Float64){
        ds_pnts->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, 2, pnts_num, arr_pnts, 2, pnts_num, GDT_Float64, 0, 0);
    }
    else{
        /// @note GDT_Int32
        int* arr_temp = new int[pnts_num * 2];
        ds_pnts->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, 2, pnts_num, arr_temp, 2, pnts_num, GDT_Int32, 0, 0);
        for(int i=0; i<pnts_num * 2; i++){
            arr_pnts[i] = arr_temp[i];
        }
        delete[] arr_temp;
    }

    GDALClose(ds_arc);
    GDALClose(ds_pnts);

    /// @note 监测pnts与arc是否匹配(防止arc_idx超限)
    for(int i=0; i<arc_num; i++)
    {
        int head_idx = arr_arc[i*2 + 0];
        int tail_idx = arr_arc[i*2 + 1];

        if(head_idx >= pnts_num || tail_idx >= pnts_num){
            delete[] arr_arc;
            delete[] arr_pnts;
            PRINT_LOGGER(logger, error, "arc_idx >= pnts_num.");
            return -3;
        }
    }

    // GDALDriver
    GDALDriver* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

    GDALDataset *ds_shp = shp_driver->Create(output_shpfile.c_str(), 0, 0, 0, GDT_Unknown, nullptr);
    if (!ds_shp) {
        delete[] arr_arc;
        delete[] arr_pnts;
        PRINT_LOGGER(logger, error, "ds_shp is nullptr.");
        return -3;
    }

    OGRSpatialReference spatialRef;
    spatialRef.SetWellKnownGeogCS("WGS84");

    OGRLayer *layer;
    if(flag_wgs84){
        layer = ds_shp->CreateLayer("network", &spatialRef, wkbLineString, nullptr);
    }
    else{
        layer = ds_shp->CreateLayer("network", nullptr, wkbLineString, nullptr);
    }
        
    if (layer == nullptr) {
        delete[] arr_arc;
        delete[] arr_pnts;
        PRINT_LOGGER(logger, error, "layer is nullptr.");
        return -3;
    }

    /// layer中新建一个名为“id”的字段, 类型是int（在属性表中显示）
    OGRFieldDefn * fieldDefn = new OGRFieldDefn("ID", OFTInteger);
    fieldDefn->SetWidth(5);
    layer->CreateField(fieldDefn);

    for (int i = 0; i < arc_num; ++i) {
        OGRLineString* poLine = new OGRLineString();
        int head_idx = arr_arc[2*i];
        int tail_idx = arr_arc[2*i+1];
        poLine->addPoint(arr_pnts[2*head_idx], arr_pnts[2*head_idx+1] * flag_geo);
        poLine->addPoint(arr_pnts[2*tail_idx], arr_pnts[2*tail_idx+1] * flag_geo);

        OGRFeature* poFeature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        poFeature->SetField("ID", i);
        poFeature->SetGeometry(poLine);

        if (layer->CreateFeature(poFeature) != OGRERR_NONE) {
            OGRFeature::DestroyFeature(poFeature);
            OGRGeometryFactory::destroyGeometry(poLine);
            delete[] arr_arc;
            delete[] arr_pnts;
            GDALClose(ds_shp);
            PRINT_LOGGER(logger, error, "Failed to create feature in shapefile.");
            return -1;
        }

        OGRFeature::DestroyFeature(poFeature);
        OGRGeometryFactory::destroyGeometry(poLine);
    }

    GDALClose(ds_shp);
    
    delete[] arr_arc;
    delete[] arr_pnts;

    PRINT_LOGGER(logger, info, "create_linestring_shp finished.");
    return 0;
}

int psnetwork_to_shapefile(const char* arc_file, const char* pspnt_file, const char* shp_file)
{
    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_arc = (GDALDataset*)GDALOpen(arc_file, GA_ReadOnly);
    if(!ds_arc){
        std::cerr<<"ds_arc is nullptr"<<std::endl;
        return -1;
    }
    int arc_num = ds_arc->GetRasterYSize();
    int arc_width = ds_arc->GetRasterXSize();
    if(arc_width != 2){
        GDALClose(ds_arc);
        std::cerr<<"arc_width != 2"<<std::endl;
        return -2;
    }
    GDALDataType arc_datatype = ds_arc->GetRasterBand(1)->GetRasterDataType();
    if(arc_datatype != GDT_Int32){
        GDALClose(ds_arc);
        std::cerr<<"arc_datatype != GDT_Int32"<<std::endl;
        return -2;
    }

    GDALDataset* ds_pnts = (GDALDataset*)GDALOpen(pspnt_file, GA_ReadOnly);
    if(!ds_pnts){
        GDALClose(ds_arc);
        std::cerr<<"ds_pnts is nullptr"<<std::endl;
        return -1;
    }
    int pnts_num = ds_pnts->GetRasterYSize();
    int pnts_width = ds_pnts->GetRasterXSize();
    if(pnts_width != 2){
        GDALClose(ds_arc);
        GDALClose(ds_pnts);
        std::cerr<<"pnts_width != 2"<<std::endl;
        return -2;
    }
    GDALDataType pnts_datatype = ds_pnts->GetRasterBand(1)->GetRasterDataType();
    if(pnts_datatype != GDT_Int32 && pnts_datatype != GDT_Float64){
        GDALClose(ds_arc);
        GDALClose(ds_pnts);
        std::cerr<<"pnts_datatype != GDT_Int32 && arc_datatype != GDT_Float64"<<std::endl;
        return -2;
    }

    /// 假设数组是整型的
    int* arr_arc = new int[arc_num * 2];
    double* arr_pnts = new double[pnts_num * 2];
    ds_arc->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, 2, arc_num, arr_arc, 2, arc_num, GDT_Int32, 0, 0);
    if(pnts_datatype == GDT_Float64){
        ds_pnts->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, 2, pnts_num, arr_pnts, 2, pnts_num, GDT_Float64, 0, 0);
    }
    else{
        /// @note GDT_Int32
        int* arr_temp = new int[pnts_num * 2];
        ds_pnts->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, 2, pnts_num, arr_temp, 2, pnts_num, GDT_Int32, 0, 0);
        for(int i=0; i<pnts_num * 2; i++){
            arr_pnts[i] = arr_temp[i];
        }
        delete[] arr_temp;
    }

    GDALClose(ds_arc);
    GDALClose(ds_pnts);

    /// @note 监测pnts与arc是否匹配(防止arc_idx超限)
    for(int i=0; i<arc_num; i++)
    {
        int head_idx = arr_arc[i*2 + 0];
        int tail_idx = arr_arc[i*2 + 1];

        if(head_idx >= pnts_num || tail_idx >= pnts_num){
            delete[] arr_arc;
            delete[] arr_pnts;
            std::cerr<<"arc_idx >= pnts_num"<<std::endl;
            return -3;
        }
    }

    // GDALDriver
    GDALDriver* shp_driver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

    GDALDataset *ds_shp = shp_driver->Create(shp_file, 0, 0, 0, GDT_Unknown, nullptr);
    if (!ds_shp) {
        delete[] arr_arc;
        delete[] arr_pnts;
        std::cerr<<"ds_shp is nullptr." << std::endl;
        return -3;
    }

    OGRLayer *layer = ds_shp->CreateLayer("network", nullptr, wkbLineString, nullptr);
    if (layer == nullptr) {
        std::cerr<<"layer is nullptr." << std::endl;
        return -3;
    }

    /// layer中新建一个名为“id”的字段, 类型是int（在属性表中显示）
    OGRFieldDefn * fieldDefn = new OGRFieldDefn("ID", OFTInteger);
    fieldDefn->SetWidth(5);
    layer->CreateField(fieldDefn);



    for (int i = 0; i < arc_num; ++i) {
        OGRLineString* poLine = new OGRLineString();
        int head_idx = arr_arc[2*i];
        int tail_idx = arr_arc[2*i+1];
        poLine->addPoint(arr_pnts[2*head_idx], -arr_pnts[2*head_idx+1]);
        poLine->addPoint(arr_pnts[2*tail_idx], -arr_pnts[2*tail_idx+1]);

        OGRFeature* poFeature = OGRFeature::CreateFeature(layer->GetLayerDefn());
        poFeature->SetField("ID", i);
        poFeature->SetGeometry(poLine);

        if (layer->CreateFeature(poFeature) != OGRERR_NONE) {
            std::cerr << "Failed to create feature in shapefile." << std::endl;
            OGRFeature::DestroyFeature(poFeature);
            OGRGeometryFactory::destroyGeometry(poLine);
            delete[] arr_arc;
            delete[] arr_pnts;
            GDALClose(ds_shp);
            return -1;
        }

        OGRFeature::DestroyFeature(poFeature);
        OGRGeometryFactory::destroyGeometry(poLine);
    }

    GDALClose(ds_shp);
    
    delete[] arr_arc;
    delete[] arr_pnts;
    return 0;
}