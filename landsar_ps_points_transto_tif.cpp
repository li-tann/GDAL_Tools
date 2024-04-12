#include <iostream>
#include <vector>

#include <gdal_priv.h>

using std::cout, std::endl;

#define DYNAMIC_ARR_COPY(ARR_A,ARR_B,LENGTH,TYPE) \
TYPE* B = (TYPE*)ARR_B; \
for(int i=0; i<LENGTH; i++)ARR_A[i]=B[i];

#define DYNAMIC_ARR_VALUE_COPY(ARR_A,INDEX_A,ARR_B,INDEX_B,TYPE) \
TYPE* A = (TYPE*)ARR_A; \
TYPE* B = (TYPE*)ARR_B; \
A[INDEX_A]=B[INDEX_B];

int main(int argc, char* argv[])
{
    if(argc < 6){
        cout<<"argc("<<argc<<") < 6"<<endl;
        return -1;
    }

    std::string input_filepath = argv[1];   
    int column_row = atoi(argv[2]);
    int column_col = atoi(argv[3]);

    int column_value = atoi(argv[4]);
    std::string output_filepath = argv[5];

    GDALAllRegister();

    GDALDataset* ds = (GDALDataset*)GDALOpen(input_filepath.c_str(), GA_ReadOnly);
    if(!ds){
        cout<<"ds is nullptr"<<endl;
        return -2;
    }
    int width = ds->GetRasterXSize();
    int height= ds->GetRasterYSize();
    GDALRasterBand* rb = ds->GetRasterBand(1);
    auto datatype = rb->GetRasterDataType();
    int datasize = GDALGetDataTypeSize(datatype);
    if(datatype != GDT_Int32 && datatype != GDT_Float32){
        cout<<"ds.datatype is not both gdt_int32 & gdt_float32"<<endl;
        return -2;
    }


    if(width < MAX(MAX(column_col,column_row),column_value)){
        cout<<"ds.width is less than max(column_col/row/value)."<<endl;
        return -2;
    }
    auto cal_arr_max = [](int* arr, int size){
        int max = arr[0];
        for(int i=1; i<size; i++){
            if(arr[i] > max)
                max = arr[i];
        }
        return max;
    };


    int op_width, op_height;
    int* arr_c = new int[height];
    int* arr_r = new int[height];
    void* arr_v = malloc(height * datasize);
    void* temp_arr = malloc(height * datasize);

    rb->RasterIO(GF_Read, column_col, 0, 1, height, temp_arr, 1, height, datatype, 0, 0);
    if(datatype == GDT_Int32){
        DYNAMIC_ARR_COPY(arr_c,temp_arr, height, int);
    }
    else{
        DYNAMIC_ARR_COPY(arr_c,temp_arr, height, float);
    }
    rb->RasterIO(GF_Read, column_row, 0, 1, height, temp_arr, 1, height, datatype, 0, 0);
    if(datatype == GDT_Int32){
        DYNAMIC_ARR_COPY(arr_r,temp_arr, height, int);
    }
    else{
        DYNAMIC_ARR_COPY(arr_r,temp_arr, height, float);
    }

    free(temp_arr);

    /// 若没有提供有效的value所在列数, 则将arr_v全部赋1
    if(column_value >= 0)
        rb->RasterIO(GF_Read, column_value, 0, 1, height, arr_v, 1, height, datatype, 0, 0);
    else{
        if(datatype == GDT_Int32){
            int* temp_arr_v = (int*)arr_v;
            for(int i=0; i<height; i++)
                temp_arr_v[i] = 1;
        }
        else{
            float* temp_arr_v = (float*)arr_v;
            for(int i=0; i<height; i++)
                temp_arr_v[i] = 1.f;
        }
    }

    GDALClose(ds);
        
    /// 找最大值, 确认输出tif的宽高尺寸
    op_width = cal_arr_max(arr_c, height);
    op_height= cal_arr_max(arr_r ,height);
    
    GDALDriver* dri_tif = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* ds_out = dri_tif->Create(output_filepath.c_str(), op_width, op_height, 1, datatype, NULL);
    GDALRasterBand* rb_out = ds_out->GetRasterBand(1);

    void* op_arr_value = malloc(op_width * op_height * datasize);

    if(datatype == GDT_Int32){
        for(int i=0; i< height; i++){
            int index = arr_r[i] * op_width + arr_c[i];
            DYNAMIC_ARR_VALUE_COPY(op_arr_value,index, arr_v, i, int);
        }
    }
    else{
        for(int i=0; i< height; i++){
            int index = arr_r[i] * op_width + arr_c[i];
            DYNAMIC_ARR_VALUE_COPY(op_arr_value,index, arr_v, i, float);
        }
    }
    delete[] arr_c;
    delete[] arr_r;
    free(arr_v);


    rb_out->RasterIO(GF_Write, 0, 0, op_width, op_height, op_arr_value, op_width, op_height, datatype, 0, 0);
    free(op_arr_value);

    GDALClose(ds_out);

    return 0;
}