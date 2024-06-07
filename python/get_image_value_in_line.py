import sys
from osgeo import gdal
import numpy as np
import matplotlib.pyplot as plt


if(len(sys.argv)>=3):
    imgpath=sys.argv[1]
    start=sys.argv[2]
    end=sys.argv[3]
else:
    print('''
        argv[1]: imgpath;
        argv[2]: start_point, x,y;
        argv[3]:   end_point, x,y;
        ''')

gdal.UseExceptions()

try:
    ds: gdal.Dataset=gdal.Open(imgpath)
except RuntimeError as e:
    print( 'Unable to open %s'% imgpath)
    sys.exit(1)

cols = ds.RasterXSize #图像长度
rows = ds.RasterYSize #图像宽度
data: np.ndarray = ds.ReadAsArray(0, 0, cols, rows) #转为numpy格式

[sx,sy]=start.split(',')
[ex,ey]=end.split(',')

sx=int(sx)
sy=int(sy)
ex=int(ex)
ey=int(ey)

line_size=int(np.sqrt((sx-ex)**2+(sy-ey)**2)+1)

x=np.linspace(0,line_size,line_size)
xlist=np.linspace(sx,ex,line_size)
ylist=np.linspace(sy,ey,line_size)

value=np.zeros_like(xlist)

for i in range(len(xlist)):
    px=xlist[i]
    py=ylist[i]
    temp_data=data[int(py):int(py)+2,int(px):int(px)+2]
    dx = px-int(px)
    dy = py-int(py)
    
    '''双线性插值'''
    value[i] = (1 - dy) * (1 - dx) * temp_data[0,0] + (1 - dy) * dx * temp_data[0,1] + dy * (1 - dx) * temp_data[1,0] + dy * dx * temp_data[1,1]


print("xlist({}):\n".format(xlist.shape),xlist)
print("ylist({}):\n".format(xlist.shape),ylist)
print("pixel_num_list({}):\n".format(xlist.shape),x)
print("value_list({}):\n".format(xlist.shape),value)

fig, ax = plt.subplots()
ax.plot(x, value)
plt.show()
