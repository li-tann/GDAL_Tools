#include "raster_include.h"

/*
    sub_band_extract.add_argument("mask")
        .help("raster image (dem), with short or float datatype.");

*/
#include <triangle.h>

struct point_st
{
    double x,y;
};

struct triangle_st
{
    int c[3];
};


int triangle_newwork(argparse::ArgumentParser* args, std::shared_ptr<spdlog::logger> logger)
{
    std::string input_path  = args->get<string>("mask");


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
            valid_num += arr[i*width+j];
        }
    }

    std::cout<<"valid_num: "<<valid_num<<std::endl;

    /// @note init triangulate tr1
    triangulateio tr1, tr2, tr3;
    tr1.numberofcorners = valid_num;
    tr1.numberofpoints = valid_num;
    tr1.numberofpointattributes = 0;
    tr1.pointlist = new double[2 * valid_num];


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



{
    using REAL = double;



    struct triangulateio in, mid, out, vorout;


    /* Define input points. */


    in.numberofpoints = 4;
    in.numberofpointattributes = 1;
    in.pointlist = (REAL *)malloc(in.numberofpoints * 2 * sizeof(REAL));
    in.pointlist[0] = 0.0;
    in.pointlist[1] = 0.0;
    in.pointlist[2] = 1.0;
    in.pointlist[3] = 0.0;
    in.pointlist[4] = 1.0;
    in.pointlist[5] = 10.0;
    in.pointlist[6] = 0.0;
    in.pointlist[7] = 10.0;
    in.pointattributelist = (REAL *)malloc(in.numberofpoints *
        in.numberofpointattributes *
        sizeof(REAL));
    in.pointattributelist[0] = 0.0;
    in.pointattributelist[1] = 1.0;
    in.pointattributelist[2] = 11.0;
    in.pointattributelist[3] = 10.0;
    in.pointmarkerlist = (int *)malloc(in.numberofpoints * sizeof(int));
    in.pointmarkerlist[0] = 0;
    in.pointmarkerlist[1] = 2;
    in.pointmarkerlist[2] = 0;
    in.pointmarkerlist[3] = 0;


    in.numberofsegments = 0;
    in.numberofholes = 0;
    in.numberofregions = 1;
    in.regionlist = (REAL *)malloc(in.numberofregions * 4 * sizeof(REAL));
    in.regionlist[0] = 0.5;
    in.regionlist[1] = 5.0;
    in.regionlist[2] = 7.0;            /* Regional attribute (for whole mesh). */
    in.regionlist[3] = 0.1;          /* Area constraint that will not be used. */


    printf("Input point set:\n\n");
    //report(&in, 1, 0, 0, 0, 0, 0);


    /* Make necessary initializations so that Triangle can return a */
    /*   triangulation in `mid' and a voronoi diagram in `vorout'.  */


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


    triangulate("pczAevn", &in, &mid, &vorout);


    printf("Initial triangulation:\n\n");
    //report(&mid, 1, 1, 1, 1, 1, 0);
    printf("Initial Voronoi diagram:\n\n");

    int num_vertices = mid.numberofpoints;
    std::cout<<"num_vertices: "<<num_vertices<<std::endl;

    int num_segments = mid.numberofsegments;
    std::cout<<"num_segments: "<<num_segments<<std::endl;

    int num_triangles = mid.numberoftriangles;
    std::cout<<"num_triangles: "<<num_triangles<<std::endl;

    return 1;


}



    
    std::cout<<__LINE__<<std::endl;

    int k=0;
    for(int i=0; i<height; i++)
    {
        for(int j=0; j<width; j++)
        {
            if(arr[i*width+j] == 0)
                continue;
            tr1.pointlist[k++] = j;
            tr1.pointlist[k++] = i;
        }
    }

    std::cout<<__LINE__<<std::endl;

    // triangulate("zcpqD", &tr1, &tr2, &tr3);
    // triangulate("pe", &tr1, &tr2, &tr3);
    triangulate("zc", &tr1, &tr2, &tr3);

    std::cout<<__LINE__<<std::endl;

    int num_tr1_points = tr1.numberofpoints;
    int num_tr2_points = tr1.numberofpoints;

    int num_tr2_triangles = tr2.numberoftriangles;
    triangle_st* tr = (triangle_st*)tr2.trianglelist;

    
    std::cout<<fmt::format("num_tr1_points: {}", num_tr1_points)<<std::endl;
    std::cout<<fmt::format("num_tr2_points: {}", num_tr2_points)<<std::endl;
    std::cout<<fmt::format("num_tr2_triangles: {}", num_tr2_triangles)<<std::endl;


    PRINT_LOGGER(logger, info, "import_points_extract finished.");
    return 1;
}
