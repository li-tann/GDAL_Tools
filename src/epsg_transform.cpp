
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

bool reprojectDem(const char* srcPath, const char* dstPath, int EPSG, double dstRes = 0.0);
int test2(const char* srcPath, const char* dstPath, int EPSG, double dstRes);

int epsg_transform(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    string input = args->get<string>("input");
    string output = args->get<string>("output");
    int epsg_num = args->get<int>("epsg");


    // bool b = reprojectDem(input.c_str(), output.c_str(), epsg_num, 10.0);
    bool b = test2(input.c_str(), output.c_str(), epsg_num, 10.0);
    std::cout<<(b ? "true" : "false")<<std::endl;
    return 1;

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

    OGRSpatialReference  spatial_ref_in(ds_in->GetProjectionRef());
    // auto spatial_ref_in = ds_in->GetSpatialRef();
    auto spatial_ref_in_epsg = spatial_ref_in.GetEPSGGeogCS();
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
    
    OGRCoordinateTransformation* coord_transformer = OGRCreateCoordinateTransformation(&spatial_ref_in, &spatial_ref_out);
    coord_transformer->Transform(vX.size(), vY.data(), vX.data());
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
    gdal_warp->papszWarpOptions = CSLSetNameValue(gdal_warp->papszWarpOptions, "DST_METHOD", "NO_GEOTRANSFORM");

    // gdal_warp->pTransformerArg   = GDALCreateGenImgProjTransformer(ds_in, ds_in->GetProjectionRef(), ds_out, ds_out->GetProjectionRef(), false, 0.0, 1);
    
    char** options = nullptr;
    options = CSLSetNameValue(options, "DST_METHOD", "NO_GEOTRANSFORM"); 
    gdal_warp->pTransformerArg   = GDALCreateGenImgProjTransformer2(ds_in, ds_out, options);
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
    // GDALClose(ds_in);
    // GDALClose(ds_out);

    return 0;
}

bool reprojectDem(const char* srcPath, const char* dstPath, int EPSG, double dstRes)
{
    GDALAllRegister();

    GDALDataset* poSrcDS = (GDALDataset*)GDALOpen(srcPath, GA_ReadOnly);
    if (poSrcDS == nullptr) {
        std::cerr << "无法打开源数据集" << std::endl;
        return 1;
    }

    OGRSpatialReference* poSrcSRS = poSrcDS->GetSpatialRef()->Clone();
    if (poSrcSRS == nullptr) {
        std::cerr << " 无法获取源数据集的空间参考 " << std::endl;
        GDALClose(poSrcDS);
        return 1;
    }

    OGRSpatialReference* poDstSRS = new OGRSpatialReference();
    poDstSRS->importFromEPSG(EPSG);

    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (poDriver == nullptr) {
        std::cerr << "无法获取 GeoTIFF 驱动" << std::endl;
        GDALClose(poSrcDS);
        delete poDstSRS;
        return 1;
    }

    GDALDataset* poDstDS = poDriver->Create(dstPath, poSrcDS->GetRasterXSize(),
                                            poSrcDS->GetRasterYSize(), poSrcDS->GetRasterCount(),
                                            poSrcDS->GetRasterBand(1)->GetRasterDataType(), nullptr);
    if (poDstDS == nullptr) {
        std::cerr << " 无法创建目标数据集 " << std::endl;
        GDALClose(poSrcDS);
        delete poDstSRS;
        return 1;
    }

    poDstDS->SetSpatialRef(poDstSRS);

    char* srcWkt = nullptr;
    char* dstWkt = nullptr;
    poSrcSRS->exportToWkt(&srcWkt);
    poDstSRS->exportToWkt(&dstWkt);

    GDALReprojectImage(poSrcDS, srcWkt, poDstDS, dstWkt,
                        GRA_Bilinear, 0, 0, nullptr, nullptr, nullptr);

    // GDALReprojectImage(poSrcDS, poSrcSRS->exportToWkt(), poDstDS, poDstSRS->ExportToWkt(),
    //                     GRA_Bilinear, 0, 0, nullptr, nullptr, nullptr);

    GDALClose(poSrcDS);
    GDALClose(poDstDS);
    delete poDstSRS;

    return true;
}

int test2(const char* srcPath, const char* dstPath, int EPSG, double dstRes)
{
    // 注册所有的 GDAL 驱动
    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

    // 1. 打开输入文件
    GDALDataset* poSrcDS = (GDALDataset*)GDALOpen(srcPath, GA_ReadOnly);
    if (poSrcDS == nullptr) {
        std::cerr << "cann't open file: " << srcPath << std::endl;
        return 1;
    }
    std::cout << "open src file: " << srcPath << std::endl;

    // 2. 获取源坐标系统 (SRS)
    const char* pszSrcWKT = poSrcDS->GetProjectionRef();
    if (pszSrcWKT == nullptr || strlen(pszSrcWKT) == 0) {
        std::cerr << " no invalid projection ref " << std::endl;
        GDALClose(poSrcDS);
        return 1;
    }
    OGRSpatialReference oSrcSRS;
    oSrcSRS.importFromWkt((char**)&pszSrcWKT);

    // 3. 定义目标坐标系统 (SRS)
    // 这里我们将其设置为 Web Mercator (EPSG:3857)
    OGRSpatialReference oTgtSRS;
    // 方法一: 从 EPSG 代码创建
    if (oTgtSRS.importFromEPSG(EPSG) != OGRERR_NONE) {
        std::cerr << " import from epsg failed. " << std::endl;
        GDALClose(poSrcDS);
        return 1;
    }
    // 方法二: 从 WKT 字符串创建 (如果你有WKT的话)
    // const char* pszTgtWKT = "PROJCS[\"WGS 84 / Pseudo-Mercator\", ... ]";
    // oTgtSRS.importFromWkt((char**)&pszTgtWKT);
    
    char* pszTgtWKT = nullptr;
    oTgtSRS.exportToWkt(&pszTgtWKT);
    std::cout << "target coord WKT:\n" << pszTgtWKT << std::endl;

    // 4. 计算输出图像的范围和尺寸
    // 4.1 获取源图像的地理范围
    double adfGeoTransform[6];
    auto x = poSrcDS->GetGeoTransform(adfGeoTransform);
    if (poSrcDS->GetGeoTransform(adfGeoTransform) == CE_None) {
        double min_x = adfGeoTransform[0];
        double max_y = adfGeoTransform[3];
        double max_x = min_x + adfGeoTransform[1] * poSrcDS->GetRasterXSize();
        double min_y = max_y + adfGeoTransform[5] * poSrcDS->GetRasterYSize();

        // 4.2 创建坐标转换对象
        OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(&oSrcSRS, &oTgtSRS);
        if (poCT == nullptr) {
            std::cerr << " cann't trans. " << std::endl;
            CPLFree(pszTgtWKT);
            GDALClose(poSrcDS);
            return 1;
        }

        // 4.3 将源图像的角点坐标转换到目标坐标系
        double adfMinMaxTgt[4];
        if (!poCT->Transform(1, &min_x, &max_y)) { std::cerr << " coord trans failed.  " << std::endl; return 1; }
        adfMinMaxTgt[0] = min_x; adfMinMaxTgt[1] = max_y;
        if (!poCT->Transform(1, &max_x, &min_y)) { std::cerr << " coord trans failed.  " << std::endl; return 1; }
        adfMinMaxTgt[2] = max_x; adfMinMaxTgt[3] = min_y;
        
        // 确保 min <= max
        double tgt_min_x = std::min(adfMinMaxTgt[0], adfMinMaxTgt[2]);
        double tgt_max_x = std::max(adfMinMaxTgt[0], adfMinMaxTgt[2]);
        double tgt_min_y = std::min(adfMinMaxTgt[1], adfMinMaxTgt[3]);
        double tgt_max_y = std::max(adfMinMaxTgt[1], adfMinMaxTgt[3]);

        // 4.4 定义输出图像的分辨率 (像素大小)
        // 这里我们选择与源图像大致相同的分辨率
        double dTgtResX = adfGeoTransform[1]; 
        double dTgtResY = -adfGeoTransform[5]; // Y方向通常是负的

        // 4.5 计算输出图像的宽度和高度
        int nTgtWidth = static_cast<int>((tgt_max_x - tgt_min_x) / dTgtResX + 0.5);
        int nTgtHeight = static_cast<int>((tgt_max_y - tgt_min_y) / dTgtResY + 0.5);
        
        std::cout << "output size: " << nTgtWidth << " x " << nTgtHeight << std::endl;

        // 5. 创建输出文件
        GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
        if (poDriver == nullptr) {
            std::cerr << " no gtiff driver。  " << std::endl;
            // OGRDestroyCoordinateTransformation(poCT);
            OGRCoordinateTransformation::DestroyCT(poCT);
            CPLFree(pszTgtWKT);
            GDALClose(poSrcDS);
            return 1;
        }

        // 设置输出文件的创建选项，例如压缩
        char** papszOptions = nullptr;
        papszOptions = CSLSetNameValue(papszOptions, "COMPRESS", "DEFLATE"); // 使用DEFLATE压缩
        papszOptions = CSLSetNameValue(papszOptions, "PREDICTOR", "2");      // 对浮点数或灰度图有更好的压缩率
        
        GDALDataset* poDstDS = poDriver->Create(dstPath, nTgtWidth, nTgtHeight, 
                                               poSrcDS->GetRasterCount(), 
                                               poSrcDS->GetRasterBand(1)->GetRasterDataType(), 
                                               papszOptions);
        CSLDestroy(papszOptions); // 释放选项列表

        if (poDstDS == nullptr) {
            std::cerr << "cann't create: " << dstPath << std::endl;
            // OGRDestroyCoordinateTransformation(poCT);
            OGRCoordinateTransformation::DestroyCT(poCT);
            CPLFree(pszTgtWKT);
            GDALClose(poSrcDS);
            return 1;
        }

        // 6. 设置输出文件的GeoTransform和Projection
        double adfTgtGeoTransform[6];
        adfTgtGeoTransform[0] = tgt_min_x;
        adfTgtGeoTransform[1] = dTgtResX;
        adfTgtGeoTransform[2] = 0;
        adfTgtGeoTransform[3] = tgt_max_y;
        adfTgtGeoTransform[4] = 0;
        adfTgtGeoTransform[5] = -dTgtResY; // Y方向向下为负

        poDstDS->SetGeoTransform(adfTgtGeoTransform);
        poDstDS->SetProjection(pszTgtWKT);

        // 7. 执行重投影和重采样
        std::cout << "reprojection..." << std::endl;
        CPLErr eErr = GDALReprojectImage(
            poSrcDS, pszSrcWKT, 
            poDstDS, pszTgtWKT,
            GRA_Bilinear, // 使用双线性插值，这是一个很好的默认选择
            0.0, 0.0,
            nullptr, nullptr, nullptr // 进度条和错误回调，可以保持为nullptr
        );

        if (eErr == CE_None) {
            std::cout << "reprojection sussess: " << dstPath << std::endl;
        } else {
            std::cerr << "reprojection failed." << std::endl;
        }
        
        // 8. 清理资源
        // OGRDestroyCoordinateTransformation(poCT);
        OGRCoordinateTransformation::DestroyCT(poCT);
        CPLFree(pszTgtWKT);
        GDALClose(poSrcDS);
        GDALClose(poDstDS);

    } else {
        std::cerr << " cann't get GeoTransform from srcDS.  " << std::endl;
        GDALClose(poSrcDS);
        return 1;
    }

    return 0;
}