#include "raster_include.h"

#include <gdal.h>
#include <gdal_priv.h>
#include <ogrsf_frmts.h>
#include <gdalwarper.h>
/*
        sub_over_resample.add_argument("input_imgpath")
            .help("input image filepath");

        sub_over_resample.add_argument("output_imgpath")
            .help("output image filepath");
            
        sub_over_resample.add_argument("scale")
            .help("scale of over-resample.")
            .scan<'g',double>();

         sub_over_resample.add_argument("method")
            .help("over-resample method, use int to represent method : 0,nearst; 1,bilinear; 2,cubic; 3,cubicSpline; 4,lanczos(sinc).; 5,average.")
            .scan<'i',int>()
            .default_value("1");     
*/

funcrst gdal_image_resample_warp(const char *pszSrcFile, const char *pszDstFile,
                                                     double scale,
                                                     GDALResampleAlg eResampleMethod = GRA_NearestNeighbour);

int over_resample(argparse::ArgumentParser* args,std::shared_ptr<spdlog::logger> logger)
{
    double scale = args->get<double>("scale");
    int method_int = args->get<int>("method");
    string input_filepath = args->get<string>("input_imgpath");
    string output_filepath = args->get<string>("output_imgpath");
    GDALResampleAlg eResampleMethod = GDALResampleAlg(method_int);

    auto rst = gdal_image_resample_warp(input_filepath.c_str(), output_filepath.c_str(), scale, eResampleMethod);
    if(!rst){
        PRINT_LOGGER(logger, error, fmt::format("gdal_image_resample_warp failed, cause : '{}'", rst.explain));
        return -1;
    }
    PRINT_LOGGER(logger, info, rst.explain);
    PRINT_LOGGER(logger, info, "over_resample success.");
    return 1;
}

funcrst gdal_image_resample_warp(const char *pszSrcFile, const char *pszDstFile,
                                                     double scale,
                                                     GDALResampleAlg eResampleMethod)
{    

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");
    CPLErr cpl_err;

    GDALDataset* pSrcDS = static_cast<GDALDataset*>(GDALOpen(pszSrcFile,/*GA_ReadOnly*/GA_Update));
    if(pSrcDS == nullptr){
        return funcrst(false, "pSrcDS is nullptr");
    }
    GDALDataType eDT = pSrcDS->GetRasterBand(1)->GetRasterDataType();

    int iBandCount = pSrcDS->GetRasterCount();
    int iSrcWidth = pSrcDS->GetRasterXSize();
    int iSrcHeight = pSrcDS->GetRasterYSize();

    //根据采样比例计算重采样后的图像宽高
    int iDstWidth = static_cast<int>(iSrcWidth * scale +0.5);
    int iDstHeight = static_cast<int>(iSrcHeight * scale +0.5);

    char *pszSrcWKT = nullptr;
    pszSrcWKT = const_cast<char *>(pSrcDS->GetProjectionRef());

    double adfGeoTransform[6]={0};
    pSrcDS->GetGeoTransform(adfGeoTransform);
    bool bNoGeoRef=false;
    double dOldGeoTrans0 = adfGeoTransform[0];
    cout<<"srcGT6:";
    for(int i=0; i<6; i++){cout<<adfGeoTransform[i]<<", ";}
    cout<<endl;
    if(strlen(pszSrcWKT)<=0){
        //如果没有投影坐标系则修改坐标系的一个值，使它不等于{0,1,0,0,0,1}，并在最后为重采样后的数据修改为原值
        //修改的是X方向的起始偏移量

        // adfGeoTransform[0]=1.0;
        adfGeoTransform[5]=-1.0;
        cpl_err = pSrcDS->SetGeoTransform(adfGeoTransform);
        bNoGeoRef=true;
    }

    //计算采样后的图像分辨率
    double dOldGeoTrans1 = adfGeoTransform[1];
    double dOldGeoTrans5 = adfGeoTransform[5];
    adfGeoTransform[1] /= scale;
    adfGeoTransform[5] /= scale;

    //创建输出文件并设置空间参考和坐标信息
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* pDstDS = poDriver->Create(pszDstFile, iDstWidth, iDstHeight, iBandCount, eDT, NULL);
    pDstDS->SetGeoTransform(adfGeoTransform);
    pDstDS->SetProjection(pSrcDS->GetProjectionRef());

    //构建坐标转换关系
    void *hTransformArg = nullptr;
    hTransformArg = GDALCreateGenImgProjTransformer2((GDALDatasetH)pSrcDS, (GDALDatasetH)pDstDS, NULL);
    if(hTransformArg == nullptr){
        GDALClose((GDALDatasetH)pSrcDS);
        GDALClose((GDALDatasetH)pDstDS);
        return funcrst(false, "GDALCreateGenImgProjTransformer2 error"); //Error -3, GDALCreateGenImgProjTransformer2 error
    }

    GDALTransformerFunc pfnTransformer = GDALGenImgProjTransform;

    //构造GDALWarp的变换选项
    GDALWarpOptions *psWO = GDALCreateWarpOptions();

    psWO->papszWarpOptions = CSLDuplicate(NULL);
    psWO->eWorkingDataType = eDT;
    psWO->eResampleAlg = eResampleMethod;

    psWO->hSrcDS = (GDALDatasetH)pSrcDS;
    psWO->hDstDS = (GDALDatasetH)pDstDS;

    psWO->pfnTransformer = pfnTransformer;
    psWO->pTransformerArg = hTransformArg;

//    psWO->pfnProgress = ALGTermProgress;
//    psWO->pProgressArg = pProcess;

    psWO->nBandCount = iBandCount;
    psWO->panSrcBands = (int*) CPLMalloc(psWO->nBandCount * sizeof (int));
    psWO->panDstBands = (int*) CPLMalloc(psWO->nBandCount * sizeof (int));
    for(int i=0;i<iBandCount;i++){
        psWO->panSrcBands[i] = i+1;
        psWO->panDstBands[i] = i+1;
    }

    //创建GDALWarp执行对象并使用GDALWarpOptions来进行初始化
    GDALWarpOperation oWO;
    oWO.Initialize(psWO);

    //执行处理
    oWO.ChunkAndWarpImage(0, 0, iDstWidth, iDstHeight);

    //释放资源和关闭文件
    GDALDestroyGenImgProjTransformer(psWO->pTransformerArg);
    GDALDestroyWarpOptions(psWO);

    if(bNoGeoRef){
        adfGeoTransform[0] = dOldGeoTrans0;
        pDstDS->SetGeoTransform(adfGeoTransform);

        adfGeoTransform[1] = dOldGeoTrans1;
        adfGeoTransform[5] = dOldGeoTrans5;
        pSrcDS->SetGeoTransform(adfGeoTransform);
    }
    GDALFlushCache(pDstDS);

    GDALClose((GDALDatasetH)pSrcDS);
    GDALClose((GDALDatasetH)pDstDS);

    return funcrst(true,"gdal_image_resample_warp success."); //INFO, function \"gdal_image_resample_warp(const char* version)\" ends normally

}