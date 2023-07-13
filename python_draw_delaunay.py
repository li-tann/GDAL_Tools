import matplotlib.pyplot as plt

p_in = []

with open("in.txt", "r", encoding='utf-8') as f:  #打开文本
    while  True:
        # Get next line from file
        line  =  f.readline()
        # If line is empty then end of file reached
        if  not  line  :
            break
        # print(line.strip())
        x = line.strip().split(",")
        p_in.append([float(x[0]),float(x[1])])

p_out = []

with open("out-2.txt", "r", encoding='utf-8') as f:  #打开文本
    while  True:
        # Get next line from file
        line  =  f.readline()
        # If line is empty then end of file reached
        if  not  line  :
            break
        # print(line.strip())
        x = line.strip().split(",")
        p_out.append([int(x[0]),int(x[1]),int(x[2])])

# plot
fig, ax = plt.subplots()

p_x = []
p_y = []

for pos in p_in:
    p_x.append(pos[0])
    p_y.append(pos[1])

ax.scatter(p_x,p_y)


for tri in p_out:
    p_xx = []
    p_yy = []
    for i in range(0,3):
        p_xx.append(p_in[tri[i]][0])
        p_yy.append(p_in[tri[i]][1])
    p_xx.append(p_in[tri[0]][0])
    p_yy.append(p_in[tri[0]][1])
    ax.plot(p_xx, p_yy)



plt.show()
