'''
reference
https://blog.csdn.net/zhang_gang1989/article/details/72884662
https://blog.csdn.net/weixin_38215395/article/details/78679296
'''
#change lineArr[1] the index accoring to the data trace format
def ReadDelayData(fileName):
    sum_owd=0.0;
    sum_lines=0;    
    with open(fileName) as txtData:
        for line in txtData.readlines():
            lineArr = line.strip().split()
            sum_owd+=float(lineArr[1])
            sum_lines+=1;
    return  sum_owd,sum_lines
    
instance=1
flows=3;
fileout="webrtc_owd.txt"    
name="%s_webrtc_%s_delay.txt"
fout=open(fileout,'w')
for case in range(instance):
    total_owd=0.0
    total_lines=0
    average_owd=0.0
    for i in range(flows):
        sum_delay=0.0
        sum_lines=0
        average_owd=0.0
        filename=name%(str(case+1),str(i+1))
        sum_delay,sum_lines=ReadDelayData(filename)
        total_owd+=sum_delay
        total_lines+=sum_lines
    average_owd=total_owd/total_lines
    fout.write(str(case+1)+"\t")
    fout.write(str(average_owd)+"\n")  
fout.close()        
