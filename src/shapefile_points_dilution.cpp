#include "vector_include.h"

/*
    sub_point_shp_dilution.add_argument("point_shapefile")
        .help("input points shapefile.");

    sub_point_shp_dilution.add_argument("ref_dem")
        .help("reference DEM.");

    sub_point_shp_dilution.add_argument("diluted_shapefile")
        .help("diluted point shapefile.");
*/
int points_shapefile_dilution(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string shp_in = args->get<string>("point_shapefile");
    string ref_dem = args->get<string>("ref_dem");
    string shp_out = args->get<string>("diluted_shapefile");

    std::cout<<fmt::format("shapefile  in: '{}'", shp_in)<<std::endl;
    std::cout<<fmt::format("reference dem: '{}'", ref_dem)<<std::endl;
    std::cout<<fmt::format("shapefile out: '{}'", shp_out)<<std::endl;

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

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

    OGRFeatureDefn* defn = layer->GetLayerDefn();
    /// @note field数量
    int fieldCount = defn->GetFieldCount();

    std::cout << "Fields:" << std::endl;
    for (int i = 0; i < fieldCount; ++i) {
        OGRFieldDefn* fieldDefn = defn->GetFieldDefn(i);
        std::cout << "  " << fieldDefn->GetNameRef() << std::endl;
        std::cout << "  " << OGRFieldDefn::GetFieldTypeName(fieldDefn->GetType()) << std::endl;
    }

    // 获取Feature数量
    int featureCount = layer->GetFeatureCount();
    std::cout << "Total Features: " << featureCount << std::endl;

    int feature_iter = 0;
    OGRFeature* poFeature;
    while ((poFeature = layer->GetNextFeature()) != nullptr && feature_iter < 10)
    {
        ++feature_iter;

        
        OGRGeometry* poGeometry = poFeature->GetGeometryRef();
        if (poGeometry == nullptr || wkbFlatten(poGeometry->getGeometryType()) != wkbPoint){
            continue;
        }

        OGRPoint* poPoint = (OGRPoint*)poGeometry;
        std::cout << "Point: (" << poPoint->getX() << ", " << poPoint->getY() << ")" << std::endl;

        // 遍历所有字段并获取字段值
        for (int iField = 0; iField < fieldCount; ++iField)
        {
            OGRFieldDefn* poFieldDefn = defn->GetFieldDefn(iField);
            std::string fieldName = poFieldDefn->GetNameRef();

            if (!poFeature->IsFieldSet(iField)) {
                std::cout << "  " << fieldName << ": (unset)" << std::endl;
                continue;
            }

            switch (poFieldDefn->GetType()) {
                case OFTInteger:
                    std::cout << "  " << fieldName << ": " << poFeature->GetFieldAsInteger(iField) << std::endl;
                    break;
                case OFTInteger64:
                    std::cout << "  " << fieldName << ": " << poFeature->GetFieldAsInteger64(iField) << std::endl;
                    break;
                case OFTReal:
                    std::cout << "  " << fieldName << ": " << poFeature->GetFieldAsDouble(iField) << std::endl;
                    break;
                case OFTString:
                    std::cout << "  " << fieldName << ": " << poFeature->GetFieldAsString(iField) << std::endl;
                    break;
                default:
                    std::cout << "  " << fieldName << ": " << poFeature->GetFieldAsString(iField) << std::endl;
                    break;
            }
        }

    }
    // 释放Feature
    OGRFeature::DestroyFeature(poFeature);
    
    GDALClose(ds);
    PRINT_LOGGER(logger, info, "points_shapefile_dilution finished.");
    return 1;
}