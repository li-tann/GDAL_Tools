
/// 1. epsg:1 --> epsg:2

/// 2. cgcs2000 + zone_width + central lontitude -> epsg:[num]

#include "raster_include.h"

/*
    sub_pix2geo.add_argument("input")
        .help("input geo-data.");
        
    sub_pix2geo.add_argument("epsg")
        .help("epsg number.");

    sub_pix2geo.add_argument("output")
        .help("transfered geo-data");
*/

#include <gdalwarper.h>

int epsg_transform(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string input = args->get<string>("input");
    string output = args->get<string>("output");
    int epsg_num = args->get<int>("epsg");

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    GDALDataset* ds_in = (GDALDataset*)GDALOpen(input.c_str(), GA_ReadOnly);
    if(!ds_in){
        PRINT_LOGGER(logger, error, "ds_in is nullptr.");
        return -1;
    }

    int width = ds_in->GetRasterXSize();
    int height= ds_in->GetRasterYSize();
    int band  = ds_in->GetRasterCount();
    auto datatype = ds_in->GetRasterBand(1)->GetRasterDataType();

    auto spatial_ref_in = ds_in->GetSpatialRef();
    auto spatial_ref_in_epsg = spatial_ref_in->GetEPSGGeogCS();
    double gt_in[6];
    ds_in->GetGeoTransform(gt_in);
    GDALClose(ds_in);
    ds_in = nullptr;

    PRINT_LOGGER(logger, info, fmt::format("ds_in.epsg: {}.", spatial_ref_in_epsg));

    OGRSpatialReference  spatial_ref_out;
    if(spatial_ref_out.importFromEPSG(epsg_num) !=CPLErr::CE_None){
        PRINT_LOGGER(logger, error, fmt::format("invalid epsg_num: {}.", epsg_num));
        return -1;
    }

    /// TODO 需要考虑如何进行单位的转换
    /// @note 计算gt_out;
    std::vector<double> vX = {gt_in[0], gt_in[0], gt_in[0]+ width*gt_in[1], gt_in[0]+ width*gt_in[1]};
    std::vector<double> vY = {gt_in[3], gt_in[3]+height*gt_in[5], gt_in[3], gt_in[3]+height*gt_in[5]};
    
    OGRCoordinateTransformation* coord_transformer = OGRCreateCoordinateTransformation(spatial_ref_in, &spatial_ref_out);
    coord_transformer->Transform(vX.size(), vX.data(), vY.data());
    OGRCoordinateTransformation::DestroyCT(coord_transformer);

    // // 计算目标范围和尺寸
    // double dfTgtMinX = *std::min_element(vX.begin(), vX.end());
    // double dfTgtMaxX = *std::max_element(vX.begin(), vX.end());
    // double dfTgtMinY = *std::min_element(vY.begin(), vY.end());
    // double dfTgtMaxY = *std::max_element(vY.begin(), vY.end());

    // double dfSrcResX = adfSrcGeoTransform[1];
    // double dfSrcResY = fabs(adfSrcGeoTransform[5]);
    // int nTgtCols = static_cast<int>((dfTgtMaxX - dfTgtMinX) / dfSrcResX + 0.5);
    // int nTgtRows = static_cast<int>((dfTgtMaxY - dfTgtMinY) / dfSrcResY + 0.5);

    // if (nTgtCols <= 0 || nTgtRows <= 0) {
    //     std::cerr << "无效的输出尺寸" << std::endl;
    //     GDALDestroyWarpOptions(psWarpOpts);
    //     GDALClose((GDALDatasetH)poSrcDS);
    //     return false;
    // }
    


    // 创建Warp选项
    GDALWarpOptions* gdal_warp = GDALCreateWarpOptions();
    if (!gdal_warp) {
        PRINT_LOGGER(logger, error, "gdal_warp is nullptr.");
        return -2;
    }

    GDALDriver* driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = driver->Create(output.c_str(), width, height, band, datatype, nullptr);
    if(!ds_out){
        CPLFree(gdal_warp);
        PRINT_LOGGER(logger, error, "ds_out is nullptr.");
        return -1;
    }
    ds_out->SetSpatialRef(spatial_ref_out.Clone());

    ds_in = (GDALDataset*)GDALOpen(input.c_str(), GA_ReadOnly);


    gdal_warp->hSrcDS            = ds_in;
    gdal_warp->hDstDS            = ds_out;
    gdal_warp->nBandCount        = band;
    gdal_warp->panSrcBands       = static_cast<int*>(CPLMalloc(sizeof(int) * gdal_warp->nBandCount));
    gdal_warp->panDstBands       = static_cast<int*>(CPLMalloc(sizeof(int) * gdal_warp->nBandCount));
    for (int i = 0; i < gdal_warp->nBandCount; ++i) {
        gdal_warp->panSrcBands[i] = i + 1;
        gdal_warp->panDstBands[i] = i + 1;
    }
    gdal_warp->eResampleAlg      = GRA_Bilinear;  // 可改为 GRA_Bilinear 等
    // gdal_warp->padfSrcNoDataReal = nullptr;
    // gdal_warp->padfDstNoDataReal = nullptr;
    // gdal_warp->papszWarpOptions  = CSLSetNameValue(nullptr, "INIT_DEST", "0");
    // gdal_warp->pfnProgress       = GDALTermProgress;
    gdal_warp->pTransformerArg   = GDALCreateGenImgProjTransformer(ds_in, ds_in->GetProjectionRef(), ds_out, ds_out->GetProjectionRef(), false, 0.0, 1);
    gdal_warp->pfnTransformer    = GDALGenImgProjTransform;

    // 6. 执行重投影
    GDALWarpOperation oOperation;
    if(oOperation.Initialize(gdal_warp) != CE_None){
        GDALDestroyWarpOptions(gdal_warp);
        GDALClose(ds_in);
        GDALClose(ds_out);
        remove(output.c_str());
        PRINT_LOGGER(logger, error, "oOperation.Initialize failed.");
        return -1;
    }

    if (oOperation.ChunkAndWarpImage(0, 0, width, height) != CE_None){
        GDALDestroyWarpOptions(gdal_warp);
        GDALClose(ds_in);
        GDALClose(ds_out);
        remove(output.c_str());
        PRINT_LOGGER(logger, error, "oOperation.ChunkAndWarpImage failed.");
        return -1;
    }

    // 7. 写入目标投影
    ds_out->SetSpatialRef(&spatial_ref_out);

    // 8. 清理
    GDALDestroyWarpOptions(gdal_warp);
    GDALClose(ds_in);
    GDALClose(ds_out);

    return 0;
}