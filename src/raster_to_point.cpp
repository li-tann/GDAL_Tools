/**
 * @file raster_to_point.cpp
 * @author li-tann (li-tann@github.com)
 * @brief convert the pixels of a raster into a point vector (shp / kml / geojson).
 * @version 0.1
 * @date 2024-05-01
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "vector_include.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>

/*
    sub_raster_to_point.add_argument("raster_image")
        .help("input raster image, only short int (Int16) and float (Float32) data types are supported.");

    sub_raster_to_point.add_argument("output_vector")
        .help("output vector filepath, only .shp, .kml and .geojson are supported.");

    sub_raster_to_point.add_argument("-l", "--left") ...
    sub_raster_to_point.add_argument("-r", "--right") ...
    sub_raster_to_point.add_argument("-t", "--top") ...
    sub_raster_to_point.add_argument("-d", "--down") ...
    sub_raster_to_point.add_argument("-g", "--geo").flag();
*/

namespace {

/// @brief deleter of GDALDataset, close the dataset whatever the return point is.
struct dataset_closer {
    void operator()(GDALDataset* ds) const {
        if (ds != nullptr) {
            GDALClose(ds);
        }
    }
};
using dataset_ptr = std::unique_ptr<GDALDataset, dataset_closer>;

std::string lower_extension(const std::string& filepath) {
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

bool nodata_equals(double value, double nodata) {
    if (std::isnan(nodata)) {
        return std::isnan(value);
    }
    if (nodata == 0.0) {
        return value == 0.0;
    }
    return std::fabs(value - nodata) <= std::fabs(nodata) * 1e-6;
}

/// @brief whether the value is NaN / Inf or equals to the NoData value.
bool is_invalid_value(double value, bool has_nodata, double nodata) {
    if (!std::isfinite(value)) {
        return true;
    }
    if (!has_nodata) {
        return false;
    }
    return nodata_equals(value, nodata);
}

} // namespace

int raster_to_point(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    auto time_start = std::chrono::system_clock::now();

    const string raster_path = args->get<string>("raster_image");
    const string vector_path = args->get<string>("output_vector");
    const bool use_geo = args->is_used("--geo");

    /// 1. output driver, decided by the extension of output_vector.
    std::string vector_driver;
    const std::string ext = lower_extension(vector_path);
    if (ext == ".shp") {
        vector_driver = "ESRI Shapefile";
    }
    else if (ext == ".kml") {
        vector_driver = "LIBKML";
    }
    else if (ext == ".geojson" || ext == ".json") {
        vector_driver = "GeoJSON";
    }
    else {
        PRINT_LOGGER(logger, error, fmt::format("unknown output extension '{}', only .shp .kml .geojson are supported.", ext));
        return -1;
    }

    GDALAllRegister();
    OGRRegisterAll();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    /// 2. open the raster image.
    dataset_ptr poDataset(static_cast<GDALDataset*>(
        GDALOpenEx(raster_path.c_str(), GDAL_OF_RASTER | GDAL_OF_READONLY, nullptr, nullptr, nullptr)));
    if (poDataset == nullptr) {
        PRINT_LOGGER(logger, error, fmt::format("open raster '{}' failed.", raster_path));
        return -2;
    }

    const int width = poDataset->GetRasterXSize();
    const int height = poDataset->GetRasterYSize();
    const int band_count = poDataset->GetRasterCount();
    if (width < 1 || height < 1 || band_count < 1) {
        PRINT_LOGGER(logger, error, fmt::format("raster size is invalid: {} x {}, {} band(s).", width, height, band_count));
        return -2;
    }
    if (band_count > 250) {
        PRINT_LOGGER(logger, error, fmt::format("{} bands are too many for a shapefile (the dbf file holds 255 fields at most).", band_count));
        return -2;
    }

    /// 3. one-stage only supports short int and float.
    const GDALDataType datatype = poDataset->GetRasterBand(1)->GetRasterDataType();
    if (datatype != GDT_Int16 && datatype != GDT_Float32) {
        PRINT_LOGGER(logger, error, "datatype is neither short int (Int16) nor float (Float32).");
        return -2;
    }
    for (int b = 1; b <= band_count; b++) {
        if (poDataset->GetRasterBand(b)->GetRasterDataType() != datatype) {
            PRINT_LOGGER(logger, error, fmt::format("band {} is not the same datatype as band 1.", b));
            return -2;
        }
    }

    /// 4. geotransform and AreaOrPoint.
    double gt[6] = { 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    const bool has_geo = poDataset->GetGeoTransform(gt) == CE_None;
    const char* area_or_point = poDataset->GetMetadataItem("AREA_OR_POINT");
    const bool pixel_is_area = (area_or_point == nullptr || !EQUAL(area_or_point, "Point"));

    /// 5. resolve the pixel window [col_min, col_max] x [row_min, row_max], both inclusive.
    double col_min_raw = 0.0;
    double col_max_raw = width - 1.0;
    double row_min_raw = 0.0;
    double row_max_raw = height - 1.0;

    if (use_geo) {
        if (!has_geo) {
            PRINT_LOGGER(logger, error, "'--geo' is used but the raster has no georeference information.");
            return -2;
        }

        double inv_gt[6];
        if (!GDALInvGeoTransform(gt, inv_gt)) {
            PRINT_LOGGER(logger, error, "the geotransform of the raster is not invertible, so '--geo' cannot be used.");
            return -2;
        }

        /// bounding box of the whole raster, in the coordinate system of the raster.
        double corner_x[4];
        double corner_y[4];
        GDALApplyGeoTransform(gt, -0.5, -0.5, &corner_x[0], &corner_y[0]);
        GDALApplyGeoTransform(gt, width - 0.5, -0.5, &corner_x[1], &corner_y[1]);
        GDALApplyGeoTransform(gt, -0.5, height - 0.5, &corner_x[2], &corner_y[2]);
        GDALApplyGeoTransform(gt, width - 0.5, height - 0.5, &corner_x[3], &corner_y[3]);

        double geo_x_min = *std::min_element(corner_x, corner_x + 4);
        double geo_x_max = *std::max_element(corner_x, corner_x + 4);
        double geo_y_min = *std::min_element(corner_y, corner_y + 4);
        double geo_y_max = *std::max_element(corner_y, corner_y + 4);

        /// a boundary which is not given keeps the border of the raster.
        if (args->is_used("--left")) {
            geo_x_min = args->get<double>("--left");
        }
        if (args->is_used("--right")) {
            geo_x_max = args->get<double>("--right");
        }
        if (args->is_used("--top")) {
            geo_y_max = args->get<double>("--top");
        }
        if (args->is_used("--down")) {
            geo_y_min = args->get<double>("--down");
        }

        /// convert the four corners of the geo range, so that a rotated geotransform is also covered.
        double cols[4];
        double rows[4];
        GDALApplyGeoTransform(inv_gt, geo_x_min, geo_y_min, &cols[0], &rows[0]);
        GDALApplyGeoTransform(inv_gt, geo_x_max, geo_y_min, &cols[1], &rows[1]);
        GDALApplyGeoTransform(inv_gt, geo_x_min, geo_y_max, &cols[2], &rows[2]);
        GDALApplyGeoTransform(inv_gt, geo_x_max, geo_y_max, &cols[3], &rows[3]);

        col_min_raw = *std::min_element(cols, cols + 4);
        col_max_raw = *std::max_element(cols, cols + 4);
        row_min_raw = *std::min_element(rows, rows + 4);
        row_max_raw = *std::max_element(rows, rows + 4);

        PRINT_LOGGER(logger, info, fmt::format("geo range: left={:.10g} right={:.10g} top={:.10g} down={:.10g}",
            geo_x_min, geo_x_max, geo_y_max, geo_y_min));
    }
    else {
        double left = 0.0;
        double right = width - 1.0;
        double top = 0.0;
        double down = height - 1.0;
        if (args->is_used("--left")) {
            left = args->get<double>("--left");
        }
        if (args->is_used("--right")) {
            right = args->get<double>("--right");
        }
        if (args->is_used("--top")) {
            top = args->get<double>("--top");
        }
        if (args->is_used("--down")) {
            down = args->get<double>("--down");
        }
        col_min_raw = std::min(left, right);
        col_max_raw = std::max(left, right);
        row_min_raw = std::min(top, down);
        row_max_raw = std::max(top, down);
    }

    /// pixels whose centre is inside the range are kept, so the raw value is shifted by half a pixel.
    int col_min = 0;
    int col_max = 0;
    int row_min = 0;
    int row_max = 0;
    if (use_geo) {
        col_min = static_cast<int>(std::ceil(col_min_raw - 0.5));
        col_max = static_cast<int>(std::floor(col_max_raw - 0.5));
        row_min = static_cast<int>(std::ceil(row_min_raw - 0.5));
        row_max = static_cast<int>(std::floor(row_max_raw - 0.5));
    }
    else {
        col_min = static_cast<int>(std::floor(col_min_raw));
        col_max = static_cast<int>(std::floor(col_max_raw));
        row_min = static_cast<int>(std::floor(row_min_raw));
        row_max = static_cast<int>(std::floor(row_max_raw));
    }

    if (col_max < 0 || col_min > width - 1 || row_max < 0 || row_min > height - 1) {
        PRINT_LOGGER(logger, error, fmt::format("the given range is out of the raster: columns {}..{}, rows {}..{}, raster size {} x {}.",
            col_min, col_max, row_min, row_max, width, height));
        return -2;
    }

    col_min = std::max(0, std::min(width - 1, col_min));
    col_max = std::max(0, std::min(width - 1, col_max));
    row_min = std::max(0, std::min(height - 1, row_min));
    row_max = std::max(0, std::min(height - 1, row_max));

    PRINT_LOGGER(logger, info, fmt::format("raster: {} x {}, {} band(s), datatype {}.",
        width, height, band_count, GDALGetDataTypeName(datatype)));
    PRINT_LOGGER(logger, info, fmt::format("pixel window: columns {}..{}, rows {}..{} ({} x {}){}.",
        col_min, col_max, row_min, row_max, col_max - col_min + 1, row_max - row_min + 1,
        use_geo ? " (converted from the geo range)" : ""));

    /// 6. spatial reference of the output layer.
    OGRSpatialReference raster_srs;
    OGRSpatialReference kml_srs;
    OGRSpatialReference* layer_srs = nullptr;
    OGRCoordinateTransformation* poCT = nullptr;

    if (use_geo) {
        const OGRSpatialReference* poSRS = poDataset->GetSpatialRef();
        if (poSRS == nullptr) {
            PRINT_LOGGER(logger, warn, "the raster has no CRS, the output points are written without a projection definition.");
        }
        else {
            raster_srs = *poSRS;
            raster_srs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
            layer_srs = &raster_srs;

            if (vector_driver == "LIBKML") {
                kml_srs.SetWellKnownGeogCS("WGS84");
                kml_srs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
                poCT = OGRCreateCoordinateTransformation(&raster_srs, &kml_srs);
                if (poCT == nullptr) {
                    PRINT_LOGGER(logger, warn, "cannot transform to WGS84, the .kml file keeps the raster coordinates.");
                }
                else {
                    layer_srs = &kml_srs;
                }
            }
        }
    }
    else if (vector_driver == "LIBKML") {
        PRINT_LOGGER(logger, warn, "pixel coordinates are written into .kml, a KML viewer will take them as lon/lat.");
    }

    /// 7. create the output vector dataset and its layer.
    GDALDriver* vector_driver_ptr = GetGDALDriverManager()->GetDriverByName(vector_driver.c_str());
    if (vector_driver_ptr == nullptr) {
        PRINT_LOGGER(logger, error, fmt::format("GDAL driver '{}' is unavailable.", vector_driver));
        if (poCT != nullptr) {
            OGRCoordinateTransformation::DestroyCT(poCT);
        }
        return -3;
    }

    dataset_ptr poOutDS(vector_driver_ptr->Create(vector_path.c_str(), 0, 0, 0, GDT_Unknown, nullptr));
    if (poOutDS == nullptr) {
        PRINT_LOGGER(logger, error, fmt::format("create '{}' failed.", vector_path));
        if (poCT != nullptr) {
            OGRCoordinateTransformation::DestroyCT(poCT);
        }
        return -3;
    }

    OGRLayer* layer = poOutDS->CreateLayer("points", layer_srs, wkbPoint, nullptr);
    if (layer == nullptr) {
        PRINT_LOGGER(logger, error, "create layer 'points' failed.");
        if (poCT != nullptr) {
            OGRCoordinateTransformation::DestroyCT(poCT);
        }
        return -3;
    }

    OGRFieldDefn field_id("ID", OFTInteger);
    field_id.SetWidth(9);
    layer->CreateField(&field_id);

    OGRFieldDefn field_row("ROW", OFTInteger);
    field_row.SetWidth(9);
    layer->CreateField(&field_row);

    OGRFieldDefn field_col("COL", OFTInteger);
    field_col.SetWidth(9);
    layer->CreateField(&field_col);

    for (int b = 1; b <= band_count; b++) {
        OGRFieldDefn field_val(fmt::format("VAL_{}", b).c_str(),
            datatype == GDT_Int16 ? OFTInteger : OFTReal);
        field_val.SetWidth(datatype == GDT_Int16 ? 9 : 20);
        field_val.SetPrecision(datatype == GDT_Int16 ? 0 : 10);
        layer->CreateField(&field_val);
    }

    /// 8. read the raster row by row and create features.
    std::vector<std::vector<double>> row_buffer(band_count, std::vector<double>(width, 0.0));
    std::vector<bool> has_nodata(band_count, false);
    std::vector<double> nodata_value(band_count, 0.0);
    for (int b = 1; b <= band_count; b++) {
        GDALRasterBand* band = poDataset->GetRasterBand(b);
        int nodata_success = 0;
        const double value = band->GetNoDataValue(&nodata_success);
        has_nodata[b - 1] = (nodata_success != 0);
        nodata_value[b - 1] = (nodata_success != 0) ? value : 0.0;
    }

    const bool in_transaction = layer->StartTransaction() == OGRERR_NONE;

    OGRPoint point;
    size_t feature_count = 0;
    size_t skipped_count = 0;
    size_t failed_count = 0;
    const int log_step = std::max(1, (row_max - row_min + 1) / 10);

    for (int row = row_min; row <= row_max; row++) {
        for (int b = 0; b < band_count; b++) {
            GDALRasterBand* band = poDataset->GetRasterBand(b + 1);
            if (band->RasterIO(GF_Read, 0, row, width, 1, row_buffer[b].data(), width, 1, GDT_Float64, 0, 0) != CE_None) {
                PRINT_LOGGER(logger, error, fmt::format("read band {} row {} failed.", b + 1, row));
                if (in_transaction) {
                    layer->CommitTransaction();
                }
                if (poCT != nullptr) {
                    OGRCoordinateTransformation::DestroyCT(poCT);
                }
                return -4;
            }
        }

        if ((row - row_min) % log_step == 0) {
            PRINT_LOGGER(logger, info, fmt::format("processing row {} / {}, {} feature(s) created.",
                row, row_max, feature_count));
        }

        for (int col = col_min; col <= col_max; col++) {
            bool all_invalid = true;
            for (int b = 0; b < band_count; b++) {
                if (!is_invalid_value(row_buffer[b][col], has_nodata[b], nodata_value[b])) {
                    all_invalid = false;
                    break;
                }
            }
            if (all_invalid) {
                skipped_count++;
                continue;
            }

            double x = static_cast<double>(col);
            double y = static_cast<double>(row);
            if (use_geo) {
                /// the point lays on the pixel centre, or on the pixel corner when AREA_OR_POINT = Point.
                const double pixel_x = col + (pixel_is_area ? 0.5 : 0.0);
                const double pixel_y = row + (pixel_is_area ? 0.5 : 0.0);
                GDALApplyGeoTransform(gt, pixel_x, pixel_y, &x, &y);

                if (poCT != nullptr && !poCT->Transform(1, &x, &y)) {
                    failed_count++;
                    continue;
                }
            }

            OGRFeature* poFeature = OGRFeature::CreateFeature(layer->GetLayerDefn());
            point.setX(x);
            point.setY(y);
            poFeature->SetGeometry(&point);
            poFeature->SetField(0, static_cast<int>(feature_count));
            poFeature->SetField(1, row);
            poFeature->SetField(2, col);
            for (int b = 0; b < band_count; b++) {
                /// fields are created in order: ID, ROW, COL, VAL_1 .. VAL_n
                const int field_index = 3 + b;
                const double value = row_buffer[b][col];
                if (is_invalid_value(value, has_nodata[b], nodata_value[b])) {
                    poFeature->SetFieldNull(field_index);
                }
                else if (datatype == GDT_Int16) {
                    poFeature->SetField(field_index, static_cast<int>(std::lround(value)));
                }
                else {
                    poFeature->SetField(field_index, value);
                }
            }

            if (layer->CreateFeature(poFeature) != OGRERR_NONE) {
                OGRFeature::DestroyFeature(poFeature);
                PRINT_LOGGER(logger, error, "create feature failed.");
                if (in_transaction) {
                    layer->CommitTransaction();
                }
                if (poCT != nullptr) {
                    OGRCoordinateTransformation::DestroyCT(poCT);
                }
                return -4;
            }
            OGRFeature::DestroyFeature(poFeature);
            feature_count++;
        }
    }

    if (in_transaction) {
        layer->CommitTransaction();
    }
    if (poCT != nullptr) {
        OGRCoordinateTransformation::DestroyCT(poCT);
        poCT = nullptr;
    }
    layer->SyncToDisk();

    PRINT_LOGGER(logger, info, fmt::format("create {} feature(s), skip {} invalid pixel(s), {} point(s) failed to transform.",
        feature_count, skipped_count, failed_count));
    PRINT_LOGGER(logger, info, fmt::format("raster_to_point success, output: {} ({}), spend time {}s.",
        vector_path, vector_driver, spend_time(time_start)));

    if (feature_count == 0) {
        PRINT_LOGGER(logger, warn, "no feature is created, check the range and the NoData values of the raster.");
    }
    return 1;
}
