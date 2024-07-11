import osgeo.gdal as gdal
import numpy as np
import matplotlib.pyplot as plt
import sys
import math

def print_separated_points(tifpath, outputpath):
    
    gdal.UseExceptions()
    points_list=[]

    try:
        ds_mas: gdal.Dataset=gdal.Open(tifpath, gdal.GA_ReadOnly)
    except RuntimeError as e:
        print( 'Unable to open %s'% tifpath)
        return False
    
    width = 300
    height= 300
    points_num = 2000

    '''在0~100的图像中随机取[points_num]个点'''
    np.random.seed(0)
    x = np.random.rand(points_num) * (width - 20) + 10
    y = np.random.rand(points_num) * (height - 20) + 10

    # plt.scatter(x,y)
    # plt.show()

    data = ds_mas.ReadAsArray(0, 0, width, height)

    for i in range(points_num):
        row = y[i]
        col = x[i]
        dr = row - math.floor(row)
        dc = col - math.floor(col)

        v_tl = data[math.floor(row), math.floor(col)]
        v_tr = data[math.floor(row), math.ceil(col)]
        v_dl = data[math.ceil(row), math.floor(col)]
        v_dr = data[math.ceil(row), math.ceil(col)]

        m1 = np.matrix([1-dr, dr])
        m2 = np.matrix([[v_tl,v_tr],[v_dl,v_dr]])
        m3 = np.matrix([[1-dc],[dc]])

        # print(m1)
        # print(m2)
        # print(m3)

        v = np.matmul(m1,m2)
        v = np.matmul(v,m3)
        vv = np.asarray(v)
        # print(vv)
        # print(vv[0][0])
        points_list.append([x[i],y[i],vv[0][0]])

    # print(points_list)

    with open(outputpath, 'w') as f:
        for p in points_list:
            f.write("{},{},{}\n".format(p[0],p[1],p[2]))
    return True



if __name__ == "__main__":
    sys.argv = ['get_separated_points.py',"c:/Users/lenovo/Desktop/test/mosaicDEM_v2_Cut.tif","c:/Users/lenovo/Desktop/test/points.txt"]
    tifpath = sys.argv[1]
    outpath = sys.argv[2]

    ans = print_separated_points(tifpath, outpath)
    print("print_separated_points.ans: ", ans)
    
