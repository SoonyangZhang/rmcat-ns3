'''
https://blog.csdn.net/qq_29422251/article/details/77713741
'''
def ReadLossInfo(fileName):
    count=0
#cache the last line
    line=""
    for index, line in enumerate(open(fileName,'r')):
        count += 1
    lineArr = line.strip().split()
    return count,int(lineArr[1])
instance=1
flows=3;
fileout="webrtc_loss.txt"    
name="%s_webrtc_%s_feedback.txt"
fout=open(fileout,'w')
for case in range(instance):
    total_recv=0
    total=0
    average_loss=0.0
    for i in range(flows):
        filename=name%(str(case+1),str(i+1))
        recv,max_recv=ReadLossInfo(filename)
        total_recv+=recv
        total+=max_recv
    average_loss=float(total-total_recv)/total
    fout.write(str(case+1)+"\t")
    fout.write(str(average_loss)+"\n")    
fout.close()
