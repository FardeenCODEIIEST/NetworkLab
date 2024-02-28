import matplotlib.pyplot as plt
import pandas as pd 

paths=['./TTL_2.csv','./TTL_4.csv','./TTL_8.csv','./TTL_16.csv']
imagePath=['./TTL_2_line.png','./TTL_4_line.png','./TTL_8_line.png','./TTL_16_line.png']
TTLs=[2,4,8,16]
c=0

for path in paths:
    df=pd.read_csv(path)
    df.plot(kind='line',x='Value of payload(in bytes)',y='Cumulative RTT(in microseconds)')
    plt.title(f"TTL {TTLs[c]}")
    plt.savefig(imagePath[c])
    c+=1
    plt.show()
