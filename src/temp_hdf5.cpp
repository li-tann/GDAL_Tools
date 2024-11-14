#include <gdal_priv.h>
#include <iostream>
#include <H5Cpp.h>
#include <fmt/format.h>
#include <filesystem>

int gdal_open_hdf5();
int h5cpp_open_hdf5();
int h5cpp_write_hdf5();

int main(int argc, char* argv[])
{
    // return gdal_open_hdf5();
    // return h5cpp_open_hdf5();
    return h5cpp_write_hdf5();
    return 0;
}

int gdal_open_hdf5()
{
    const char* hdf5_file = "d:/1_Data/MintPy/WellsEnvD2T399/mintpy/timeseries.h5";

    GDALAllRegister();

    GDALDataset* ds = (GDALDataset*)GDALOpen(hdf5_file, GA_ReadOnly);
    std::cout<<ds->GetRasterXSize()<<std::endl;
    std::cout<<ds->GetRasterYSize()<<std::endl;
    int nRasterCount = ds->GetRasterCount();
    std::cout << "raster count: " << nRasterCount << std::endl;

    auto meta = ds->GetMetadata();
    std::cout<<meta[0]<<std::endl;

    auto subdatasets = ds->GetMetadata("SUBDATASETS");
    auto subdatasets_count = CSLCount(subdatasets);
    std::cout<<"subdatasets_count:"<<subdatasets_count<<std::endl;
    for(int i=0; i<subdatasets_count; i++){
        std::cout<<subdatasets[i]<<std::endl;
    }

    

    std::vector<std::string> vSubDataSets,  vSubDataDesc;

    for(int i=0; subdatasets[i] != NULL; i++ )
    {
        if(i%2 != 0)
            continue;

        std::string tmpstr = std::string(subdatasets[i]);
        /// SUBDATASET_1_NAME=HDF5:"d:/1_Data/MintPy/WellsEnvD2T399/mintpy/inputs/geometryGeo.h5"://azimuthCoord

        tmpstr = tmpstr.substr(tmpstr.find_first_of("=") + 1);
        /// HDF5:"d:/1_Data/MintPy/WellsEnvD2T399/mintpy/inputs/geometryGeo.h5"://azimuthCoord
        /// 

        const char *tmpc_str = tmpstr.c_str();

        std::string tmpdsc = std::string(subdatasets[i+1]);
        tmpdsc = tmpdsc.substr(tmpdsc.find_first_of("=") + 1);

        // GDALDatasetH hTmpDt = GDALOpen(tmpc_str, GA_ReadOnly);
        // if(hTmpDt != NULL)
        // {
            
        // 	vSubDataSets.push_back(tmpstr);
        // 	vSubDataDesc.push_back(tmpdsc);

        // 	GDALClose(hTmpDt);
        // }
        GDALDataset* ds_sub = (GDALDataset*)GDALOpen(tmpc_str, GA_ReadOnly);
        if(!ds_sub)
            continue;
        std::cout<<std::endl;
        std::cout << "xsize: " <<ds_sub->GetRasterXSize()<<std::endl;
        std::cout << "ysize: " <<ds_sub->GetRasterYSize()<<std::endl;
        std::cout << "raster count: " << ds_sub->GetRasterCount() << std::endl;
        GDALRasterBand* rb = ds_sub->GetRasterBand(1);
        std::cout<< "datatype: "<<GDALGetDataTypeName(rb->GetRasterDataType())<<std::endl;
        

        double gt[6];
        auto err = ds_sub->GetGeoTransform(gt);
        if(err != CE_None){
            std::cout<<"ds_sub->GetGeoTransform(gt); failed."<<std::endl;
        }
        else{
            std::cout<<"gt :"<<gt[0]<<","<<gt[1]<<","<<gt[2]<<","<<gt[3]<<","<<gt[4]<<","<<gt[5]<<std::endl;
        }

        std::cout<<"system: "<<ds_sub->GetProjectionRef()<<std::endl;


        auto meta_sub = ds_sub->GetMetadata();
        auto metasub_count = CSLCount(meta_sub);
        std::cout<<"metasub_count:"<<metasub_count<<std::endl;
        for(int i=0; i<metasub_count; i++){
            std::cout<<meta_sub[i]<<std::endl;
        }
        
    }//end for

    return 1;
}


int h5cpp_open_hdf5()
{
    H5::Exception::dontPrint();

    const char* hdf5_file = "d:/1_Data/MintPy/WellsEnvD2T399/mintpy/timeseries.h5";
    // const char* hdf5_file = "d:/1_Data/MintPy/WellsEnvD2T399/mintpy/pbaseHistory.pdf";

    H5::H5File file;
    
    try
    {
        file.openFile(hdf5_file, H5F_ACC_RDONLY);
    }
    catch (H5::Exception& error) {
        // 捕获文件相关的异常
        // error.printError();
        
        std::cerr << "Failed to open file: " << hdf5_file << std::endl;
        std::cerr << "detail msg: "<<error.getDetailMsg()<<std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    // return 0;

    // if(!std::filesystem::exists(hdf5_file) || !std::filesystem::is_regular_file(hdf5_file)){
    //     std::cout<<"hdf5_file is not existed or not ragular file."<<std::endl;
    //     return 0;
    // }   

    // if(!H5::H5File::isHdf5(hdf5_file)){
    //     std::cout<<"hdf5_file is not a hdf5 file\n";
    //     return 0;
    // }

    
    // H5::H5File file(hdf5_file, H5F_ACC_RDONLY);

    // file.openGroup()
    std::cout<<"file.getNumObjs(): "<<file.getNumObjs()<<std::endl;
    std::cout<<"file.getObjName(): "<<file.getObjName()<<std::endl;

    using namespace H5;
    int root_attr_num = file.getNumAttrs();
    for(int i=0; i< root_attr_num; i++){
        auto attr = file.openAttribute(i);
        // std::cout<<"attr"<<i<<": "<<attr.getName()<<std::endl;


        DataType dtype = attr.getDataType();
        DataSpace dspace = attr.getSpace();
        int rank = dspace.getSimpleExtentNdims();
        hsize_t* dims = new hsize_t[rank];
        dspace.getSimpleExtentDims(dims, NULL);
        auto attrName = attr.getName();

        // 根据属性类型读取数据
        if (dtype == PredType::NATIVE_INT) {
            int attrData;
            attr.read(dtype, &attrData);
            std::cout << "Attribute " << attrName << " (int): " << attrData << std::endl;
        } else if (dtype == PredType::NATIVE_FLOAT) {
            float attrData;
            attr.read(dtype, &attrData);
            std::cout << "Attribute " << attrName << " (float): " << attrData << std::endl;
        } else if (dtype == PredType::NATIVE_DOUBLE) {
            double attrData;
            attr.read(dtype, &attrData);
            std::cout << "Attribute " << attrName << " (double): " << attrData << std::endl;
        } else if (dtype.getClass() == H5T_STRING) {
            H5std_string attrData;
            attr.read(dtype, attrData);
            std::cout << "Attribute " << attrName << " (string): " << attrData << std::endl;
        }
        
        delete[] dims;
        // 关闭属性
        attr.close();
    }
    
    for(int i=0; i<file.getNumObjs(); i++){
        // file.getObjName
        // file.openGroup();
        std::cout<<std::endl;
        std::string objName = file.getObjnameByIdx(i);
        H5G_obj_t objType = file.getObjTypeByIdx(i);
        std::cout<<"objName: "<<objName<<std::endl;
        
        if (objType == H5G_GROUP) {
            // 如果是组，打印并递归遍历
            // std::cout << "Group: " << fullPrefix << std::endl;
            std::cout<<"group\n";
            H5::Group subgroup = file.openGroup(objName);
            // listGroup(subgroup, fullPrefix);
        } else if (objType == H5G_DATASET) {
            // 如果是数据集，打印
            std::cout<<"dataset\n";
            // std::cout << "Dataset: " << fullPrefix << std::endl;
            auto dataset = file.openDataSet(objName);
            auto space = dataset.getSpace();
            auto Ndims = space.getSimpleExtentNdims();
            std::cout<<"Ndims: "<<Ndims<<std::endl;
            hsize_t* dims = new hsize_t[Ndims];
            space.getSimpleExtentDims(dims);
            std::cout<<"dims: ";
            for(int i=0; i<Ndims; i++){
                std::cout<<dims[i]<<(i == Ndims - 1 ? "\n" : ",");
            }

            auto datatype = dataset.getDataType();
            if(datatype == H5::PredType::NATIVE_FLOAT){
                std::cout<<"datatype is float\n";
            }
            // float* arr = new float[dims[1] * dims[2]];
            // dataset.read(arr, )

            int attr_nums = dataset.getNumAttrs();
            std::cout<<"attr_nums: "<<attr_nums<<std::endl;
            for(int i=0; i<attr_nums; i++){
                auto attr = dataset.openAttribute(i);
                std::cout<<"attr"<<i<<": "<<attr.getName()<<std::endl;
            }
            delete[] dims;
        }

        if(i == 0){
            /// 针对 space 1
            auto dataset = file.openDataSet(objName);
            auto space = dataset.getSpace();
            auto Ndims = space.getSimpleExtentNdims();      /// 1

            hsize_t dims[1];
            space.getSimpleExtentDims(dims);
            hsize_t offset_dims[1];
            offset_dims[0] = 5;
            hsize_t mem_dims[1];
            mem_dims[0] = 2;

            H5::DataSpace memspace(1, mem_dims); // 创建内存空间
            space.selectHyperslab(H5S_SELECT_SET, mem_dims, offset_dims);

            float data_out[2]; // 用于存储读取数据的缓冲区
            dataset.read(data_out, H5::PredType::NATIVE_FLOAT, memspace, space);

            std::cout<<data_out[0]<<","<<data_out[1]<<std::endl;
        }

        if(i == 2){
            /// 针对 space 3
            auto dataset = file.openDataSet(objName);
            auto space = dataset.getSpace();
            auto Ndims = space.getSimpleExtentNdims();      /// 3

            hsize_t dims[3];
            space.getSimpleExtentDims(dims);
            hsize_t offset_dims[3];
            offset_dims[0] = 1;offset_dims[1] = 0;offset_dims[2] = 4;
            hsize_t mem_dims[3];
            mem_dims[0] = 1;mem_dims[1] = 4;mem_dims[2] = 2;

            H5::DataSpace memspace(Ndims, mem_dims); // 创建内存空间
            space.selectHyperslab(H5S_SELECT_SET, mem_dims, offset_dims);

            float data_out[1][4][2]; // 用于存储读取数据的缓冲区
            dataset.read(data_out, H5::PredType::NATIVE_FLOAT, memspace, space);

            for(int b=0; b<1; b++)
            {
                for(int i=0; i<4; i++)
                {
                    for(int j=0; j<2; j++)
                    {
                        std::cout<<data_out[b][i][j]<<",";
                    }
                    std::cout<<std::endl;
                }
                std::cout<<std::endl;
            }
        }
    }


    file.close();
    return 1;
}


int h5cpp_write_hdf5()
{
    H5::Exception::dontPrint();

    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");

    const char* dem_filepath = "d:\\1_Data\\AP_15996_FBS_F0610_RT1.dem.tif";
    const char* h5_filepath = "d:\\1_Data\\AP_15996_FBS_F0610_RT1.dem2.h5";

    GDALDataset* ds = (GDALDataset*)GDALOpen(dem_filepath, GA_ReadOnly);
    GDALRasterBand* rb = ds->GetRasterBand(1);

    double gt[6];
    ds->GetGeoTransform(gt);
    H5std_string projectRef = ds->GetProjectionRef();

    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();

    /// datatype is short
    short* arr_dem = new short[width * height];
    rb->RasterIO(GF_Read, 0, 0, width, height, arr_dem, width, height, GDT_Int16, 0, 0);
    GDALClose(ds);

    int size = 128;
    short* block = new short[size*size];
    short** block2 = new short*[size];
    for(int i=0; i<size; i++){
        block2[i] = &(block[i*size]);
    }


    // // 创建HDF5文件
    // H5::H5File file(h5_filepath, H5F_ACC_TRUNC);


    H5::H5File* file = nullptr;
    try
    {
        file = new H5::H5File(h5_filepath, H5F_ACC_TRUNC);
    }
    catch (H5::Exception& error) {
        std::cerr << "Failed to open file: " << h5_filepath << std::endl;
        std::cerr << "detail msg: "<<error.getDetailMsg()<<std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    if(!file){
        std::cout<<"create h5file failed.\n";
        return 1;
    }

    // 定义数据集的维度
    hsize_t dims[2] = {size, size};
    H5::DataSpace dataspace(2, dims);
    
    // 定义数据类型
    H5::DataType datatype(H5::PredType::NATIVE_SHORT);

    // 创建数据集的属性列表（可选）
    H5::DSetCreatPropList cpl;
    

    hsize_t file_attrDims[1] = {6};
    H5::DataSpace file_attrSpace(1, file_attrDims);
    H5::Attribute attr_gt = file->createAttribute("geotransform", H5::PredType::NATIVE_DOUBLE, file_attrSpace);
    attr_gt.write(H5::PredType::NATIVE_DOUBLE, &gt);
    attr_gt.close();


    H5::DataType attrType_proj = H5::StrType(H5::PredType::C_S1, projectRef.size() + 1); // 字符串类型
    H5::DataSpace attrSpace_proj(H5S_SCALAR); // 标量空间
    H5::Attribute attr_proj = file->createAttribute("project", attrType_proj, attrSpace_proj);
    attr_proj.write(attrType_proj, projectRef);
    attr_proj.close();



    int idx = 0;
    for(int i=0; i<height; i+=128)
    {   if( i + 128 > height)
            continue;
        for(int j=0; j<width; j+=128)
        {
            if( j + 128 > width)
                continue;
            
            // std::cout<<"topleft: "<<i<<","<<j<<std::endl;

            /// block arr
            for(int m = 0; m< size; m++){
                for(int n = 0; n<size; n++){
                    block2[m][n] = arr_dem[(i+m)*width+(j+n)];
                }
            }
            std::string block_name = fmt::format("block-{:0>4}",idx);

            /// 创建dataset
            H5::DataSet tmp_dataset = file->createDataSet(block_name, H5::PredType::NATIVE_SHORT, dataspace, cpl);
            tmp_dataset.write(block, H5::PredType::NATIVE_SHORT);
            

            /// 创建attribute，并写入dataset内
            hsize_t attrDims[1] = {1};
            H5::DataSpace attrSpace(1, attrDims);
            H5::Attribute attrX = tmp_dataset.createAttribute("pos X", H5::PredType::NATIVE_INT, attrSpace);
            H5::Attribute attrY = tmp_dataset.createAttribute("pos Y", H5::PredType::NATIVE_INT, attrSpace);
            attrX.write(H5::PredType::NATIVE_INT, &j);
            attrY.write(H5::PredType::NATIVE_INT, &i);

            attrX.close();
            attrY.close();

            tmp_dataset.close();

            idx++;
        }
    }

    // file.close();
    file->close();
    delete file;

    


    return 2;
}