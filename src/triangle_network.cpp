#include "raster_include.h"
#include <triangle.h>

/*
    sub_band_extract.add_argument("mask")
    sub_triangle.add_argument("-f","--format").default_value("txt").choices("txt", "tif", "bin");
    sub_triangle.add_argument("-p","--pointlist").nargs(1)(*.txt, *.tif, *.pnts)
    sub_triangle.add_argument("-e","--edgelist").nargs(1)(*.txt, *.tif, *.edges)
    sub_triangle.add_argument("-t","--trianglelist").nargs(1)(*.txt, *.tif, *.tris)
*/

struct point_st{double x,y;};
struct triangle_st{int c[3];};
struct arc_st{int p[2];};

int triangle_network(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("mask");
    std::string format = args->get<string>("--format");

    bool flag_pointlist = args->is_used("--pointlist");
    bool flag_edgelist = args->is_used("--edgelist");
    bool flag_trianglelist = args->is_used("--trianglelist");

    std::string pointlist_filepath;
    std::string edgelist_filepath;
    std::string trianglelist_filepath;

    if(flag_pointlist)
        pointlist_filepath = args->get<string>("--pointlist");
    if(flag_edgelist)
        edgelist_filepath = args->get<string>("--edgelist");
    if(flag_trianglelist)
        trianglelist_filepath = args->get<string>("--trianglelist");

    PRINT_LOGGER(logger, info, fmt::format("input_path: [{}]",input_path));
    PRINT_LOGGER(logger, info, fmt::format("format: [{}]",format));
    PRINT_LOGGER(logger, info, fmt::format("pointlist({}): [{}]",flag_pointlist?"true":"false",pointlist_filepath));
    PRINT_LOGGER(logger, info, fmt::format("edgelist({}): [{}]",flag_edgelist?"true":"false",edgelist_filepath));
    PRINT_LOGGER(logger, info, fmt::format("trianglelis({}): [{}]",flag_trianglelist?"true":"false",trianglelist_filepath));


    GDALAllRegister();
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "NO");

    auto dataset = (GDALDataset*)GDALOpen(input_path.c_str(), GA_ReadOnly);
    if(!dataset){
        PRINT_LOGGER(logger, error, "dataset is nullptr.");
        return -1;
    }

    int width = dataset->GetRasterXSize();
    int height = dataset->GetRasterYSize();

    auto band = dataset->GetRasterBand(1);
    auto datatype = band->GetRasterDataType();

    if(datatype != GDT_Byte){
        PRINT_LOGGER(logger, error, "datatype is not byte.");
        GDALClose(dataset);
        return -1;
    }

    /// @note mask
    char* arr = new char[width * height];
    band->RasterIO(GF_Read, 0, 0, width, height, arr, width, height, GDT_Byte, 0, 0);
    int valid_num = 0;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            if(arr[i*width+j] != 0)
                ++valid_num;
            // valid_num += arr[i*width+j];
        }
    }

    PRINT_LOGGER(logger, info, fmt::format("valid_num: {}.", valid_num));


/*      If the `p' switch is used, `segmentlist' must point to a list of     */
/*      segments, `numberofsegments' must be properly set, and               */
/*      `segmentmarkerlist' must either be set to NULL (in which case all    */
/*      markers default to zero), or must point to a list of markers.        */
/*    - If the `p' switch is used without the `r' switch, then               */
/*      `numberofholes' and `numberofregions' must be properly set.  If      */
/*      `numberofholes' is not zero, `holelist' must point to a list of      */
/*      holes.  If `numberofregions' is not zero, `regionlist' must point to */
/*      a list of region constraints.                                        */
/*    - If the `p' switch is used, `holelist', `numberofholes',              */
/*      `regionlist', and `numberofregions' is copied to `out'.  (You can    */
/*      nonetheless get away with not initializing them if the `r' switch is */
/*      used.)                                                               */
/*    - `edgelist', `edgemarkerlist', `normlist', and `numberofedges' may be */
/*      ignored.  */

    using REAL = double;

    triangulateio in, mid, vorout;

    /// @note init triangulate in
    in.numberofpoints = valid_num;
    in.numberofpointattributes = 0;
    in.pointlist = (REAL *)malloc(in.numberofpoints * 2 * sizeof(REAL));
    int k=0;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            if(arr[i*width+j] == 0)
                continue;
            in.pointlist[k++] = j;
            in.pointlist[k++] = i;
        }
    }

    /// @note in.numberofpointattributes 可以改成0, pointattributelist 不申请内存
    in.numberofpointattributes = 0;
    in.pointattributelist = (REAL *)malloc(in.numberofpoints * in.numberofpointattributes * sizeof(REAL));

    /// @note pointmarkerlist 注释后就会报错
    in.pointmarkerlist = (int *)NULL;

    in.numberofsegments = 0;
    in.numberofholes = 0;
    in.numberofregions = 0;

    /* Make necessary initializations so that Triangle can return a */
    /*   triangulation in `mid' and a voronoi diagram in `vorout'.  */

    /// @note init triangulate mid, vorout
    mid.pointlist = (REAL *)NULL;            /* Not needed if -N switch used. */
    /* Not needed if -N switch used or number of point attributes is zero: */
    mid.pointattributelist = (REAL *)NULL;
    mid.pointmarkerlist = (int *)NULL; /* Not needed if -N or -B switch used. */
    mid.trianglelist = (int *)NULL;          /* Not needed if -E switch used. */
    /* Not needed if -E switch used or number of triangle attributes is zero: */
    mid.triangleattributelist = (REAL *)NULL;
    mid.neighborlist = (int *)NULL;         /* Needed only if -n switch used. */
    /* Needed only if segments are output (-p or -c) and -P not used: */
    mid.segmentlist = (int *)NULL;
    /* Needed only if segments are output (-p or -c) and -P and -B not used: */
    mid.segmentmarkerlist = (int *)NULL;
    mid.edgelist = (int *)NULL;             /* Needed only if -e switch used. */
    mid.edgemarkerlist = (int *)NULL;   /* Needed if -e used and -B not used. */


    vorout.pointlist = (REAL *)NULL;        /* Needed only if -v switch used. */
    /* Needed only if -v switch used and number of attributes is not zero: */
    vorout.pointattributelist = (REAL *)NULL;
    vorout.edgelist = (int *)NULL;          /* Needed only if -v switch used. */
    vorout.normlist = (REAL *)NULL;         /* Needed only if -v switch used. */


    /* Triangulate the points.  Switches are chosen to read and write a  */
    /*   PSLG (p), preserve the convex hull (c), number everything from  */
    /*   zero (z), assign a regional attribute to each element (A), and  */
    /*   produce an edge list (e), a Voronoi diagram (v), and a triangle */
    /*   neighbor list (n).                                              */

    PRINT_LOGGER(logger, info, "triangulate_command: pczAevnQ");

    triangulate("pczAevnQ", &in, &mid, &vorout);

    /*
    p: 如果输入的是点, 一般需要加上p用来生成poly线段
    c: 允许在凸包(convex hull)上创建线段 
    z: zero, 所有数据都是从零开始编号
    A: 为triangle提供一个浮点熟悉(不知道有啥用), -A只有在使用了-p但没有使用-r时才会生效
    e: 输出edge
    v: 输出Voronoi图(此处好像没用)
    n: 输出neighbor(此处好像没用)
    Q: quiet, 静默模式, 不打印任何信息
    */

    int num_vertices = mid.numberofpoints;
    PRINT_LOGGER(logger, info, fmt::format("num_vertices: {}.", num_vertices));

    // int num_segments = mid.numberofsegments;
    // std::cout<<"num_segments: "<<num_segments<<std::endl;

    int num_triangles = mid.numberoftriangles;
    PRINT_LOGGER(logger, info, fmt::format("num_triangles: {}.", num_triangles));

    int num_edges = mid.numberofedges;
    PRINT_LOGGER(logger, info, fmt::format("num_edges: {}.", num_edges));

    point_st* pointlist = (point_st*)mid.pointlist;
    triangle_st* trianglelist = (triangle_st*)mid.trianglelist;
    arc_st* edgelist = (arc_st*)mid.edgelist;


    /// @note output

    if(format == "txt")
    {
        /// @note *.txt
        if(flag_pointlist){
            /// point list
            std::ofstream ofs(pointlist_filepath);
            if(!ofs.is_open()){
                 PRINT_LOGGER(logger, error, "ofs pointlist open failed.");
            }else{
                ofs<<"point.x, point.y"<<std::endl;
                for(int i=0; i<mid.numberofpoints; i++){
                    ofs<<pointlist[i].x<<","<<pointlist[i].y<<std::endl;
                }
                ofs.close();
            }
        }
        if(flag_edgelist)
        {
            /// edge list
            std::ofstream ofs(edgelist_filepath);
            if(!ofs.is_open()){
                 PRINT_LOGGER(logger, error, "ofs edgelist open failed.");
            }else{
                ofs<<"head_idx, tail_idx"<<std::endl;
                for(int i=0; i<mid.numberofedges; i++){
                    ofs<<edgelist[i].p[0]<<","<<edgelist[i].p[1]<<std::endl;
                }
                ofs.close();
            }
        }
        if(flag_trianglelist)
        {
            /// triangle list
            std::ofstream ofs(trianglelist_filepath);
            if(!ofs.is_open()){
                 PRINT_LOGGER(logger, error, "ofs trianglelist open failed.");
            }else{
                ofs<<"t0_idx, t1_idx, t2_idx"<<std::endl;
                for(int i=0; i<mid.numberofedges; i++){
                    ofs<<trianglelist[i].c[0]<<","<<trianglelist[i].c[1]<<","<<trianglelist[i].c[2]<<std::endl;
                }
                ofs.close();
            }
        }
    }
    else if(format == "tif")
    {
        /// @note *.tif
        GDALDriver* dri = GetGDALDriverManager()->GetDriverByName("GTiff");
        if(flag_pointlist){
            /// point list
            GDALDataset* ds_pnt = dri->Create(pointlist_filepath.c_str(), 2, mid.numberofpoints, 1, GDT_Float64, NULL);
            if(!ds_pnt){
                PRINT_LOGGER(logger, warn, "ds_pnt is nullptr.");
            }
            else{
                GDALRasterBand* rb_pnt = ds_pnt->GetRasterBand(1);
                rb_pnt->RasterIO(GF_Write, 0, 0, 2, mid.numberofpoints, mid.pointlist, 2, mid.numberofpoints, GDT_Float64, 0, 0);
                GDALClose(ds_pnt);
            }
        }
        if(flag_edgelist)
        {
            /// edge list
            GDALDataset* ds_edge = dri->Create(edgelist_filepath.c_str(), 2, mid.numberofedges, 1, GDT_Int32, NULL);
            if(!ds_edge){
                PRINT_LOGGER(logger, warn, "ds_edge is nullptr.");
            }
            else{
                GDALRasterBand* rb_edge = ds_edge->GetRasterBand(1);
                rb_edge->RasterIO(GF_Write, 0, 0, 2, mid.numberofedges, mid.edgelist, 2, mid.numberofedges, GDT_Int32, 0, 0);
                GDALClose(ds_edge);
            }
        }
        if(flag_trianglelist)
        {
            /// triangle list
            GDALDataset* ds_tri = dri->Create(trianglelist_filepath.c_str(), 3, mid.numberoftriangles, 1, GDT_Int32, NULL);
            if(!ds_tri){
                PRINT_LOGGER(logger, warn, "ds_tri is nullptr.");
            }
            else{
                GDALRasterBand* rb_tri = ds_tri->GetRasterBand(1);
                rb_tri->RasterIO(GF_Write, 0, 0, 3, mid.numberoftriangles, mid.trianglelist, 3, mid.numberoftriangles, GDT_Int32, 0, 0);
                GDALClose(ds_tri);
            }
        }
    }
    else if(format == "bin")
    {
        PRINT_LOGGER(logger, error, "output with format 'bin' is building.");
        return -1;

        /// @note *.bin
        if(flag_pointlist){
            /// point list
        }
        if(flag_edgelist)
        {
            /// edge list
        }
        if(flag_trianglelist)
        {
            /// triangle list
        }
    }
    else{
        PRINT_LOGGER(logger, error, fmt::format("unknown format '{}'.", format));
        return -1;
    }

    PRINT_LOGGER(logger, info, "import_points_extract finished.");
    return 1;
}
