import json
import matplotlib.pyplot as plt
import matplotlib.patches as patches
from osgeo import gdal

'''
读取quadtree.cpp生成的四叉树json文件, 
提取所有的线段的坐标, 生成txt
方便后续使用matlab或matplotlib绘制四叉树
'''

def quadtree_json_to_line(qd_json, rect_list):
    if not(qd_json['quadtree_topleft'] == None):
        quadtree_json_to_line(qd_json['quadtree_topleft'], rect_list)
        quadtree_json_to_line(qd_json['quadtree_topright'], rect_list)
        quadtree_json_to_line(qd_json['quadtree_downleft'], rect_list)
        quadtree_json_to_line(qd_json['quadtree_downright'], rect_list)
        return 

    # rect_list.append([qd_json['start_x']-0.5, qd_json['start_y']-0.5, qd_json['width']+1, qd_json['height']+1])
    rect_list.append([qd_json['start_x'], qd_json['start_y'], qd_json['width'], qd_json['height']])


    
def load_quadtree_json(jsonpath):
    # start_list = []
    # end_list = []
    rect_list = []
    with open(jsonpath,'r+') as f:
        j = json.load(f)
        quadtree_json_to_line(j,rect_list)

    return rect_list



if __name__ == "__main__":
    '''dem'''
    dempath=r'd:\_experiments\sar_fast_geocode\test_4\dem_30m.tif'
    '''使用gdal_tool_raster的pnt_extract得到的重要点散点图'''
    '''掩模图mask.tif被使用gdal_tool_raster的quadtree调用生成四叉树json文件'''
    pntstxtpath=r'd:\_experiments\sar_fast_geocode\test_4\dem_30m.imp_pnts.txt'
    '''使用掩模图生成的四叉树json文件, 被load_quadtree_json调用生成四叉树的矩形'''
    jsonpath=r"d:\_experiments\sar_fast_geocode\test_4\dem_30m.imp_pnts.quadtree.json"
    '''如果需要输出矩形信息, 将保存到这里'''
    recttxtpath=r"d:\_experiments\sar_fast_geocode\test_4\dem_30m.imp_pnts.quadtree.rect.txt"
    
    rect_list = load_quadtree_json(jsonpath)
    print(len(rect_list))

    scatter_x = []
    scatter_y = []
    with open(pntstxtpath,'r') as f:
         strlist = f.readlines()
         for s in strlist:
            vec = s.split(',')
            #   print(vec)
            #   pnts_list.append([float(vec[1]), float(vec[0])])
            scatter_x.append(float(vec[1]))
            scatter_y.append(float(vec[0]))

    fig, ax = plt.subplots()

    '''dem'''
    # img = plt.imread(dempath)
    gdal.UseExceptions()
    dataset = gdal.Open(dempath, gdal.GA_ReadOnly)
    band = dataset.GetRasterBand(1)  # 获取第一波段(波段序号从1开始，而不是0)
    image_data = band.ReadAsArray()
    ax.imshow(image_data, cmap='gray')

    '''quadtree's rectangle'''
    for rect in rect_list:
            # rect = patches.Rectangle((rect[0], rect[1]), rect[2], rect[3], linewidth=1, edgecolor='r',facecolor='none')
            # rect = patches.Rectangle((rect[0], rect[1]), rect[2], rect[3], edgecolor='r',facecolor='none')
            rect = patches.Rectangle((rect[0], rect[1]), rect[2], rect[3], edgecolor='#ff000050',facecolor='none')
            ax.add_patch(rect)

    
    '''important points'''
    plt.scatter(scatter_x, scatter_y,s=1)


    # plt.xlim([0, 200]) 
    # plt.ylim([0, 200]) 
    # plt.gca().invert_yaxis()
    plt.show()