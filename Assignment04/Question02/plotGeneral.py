import matplotlib.pyplot as plt
import pandas as pd


csv_paths = ['./TTL_2.csv', './TTL_4.csv', './TTL_8.csv', './TTL_16.csv']
ttl_values = [2, 4, 8, 16] 

plt.figure(figsize=(10, 6))  

colors = ['blue', 'green', 'red', 'purple']

for path, ttl, color in zip(csv_paths, ttl_values, colors):
    df = pd.read_csv(path)
    plt.plot(df['Value of payload(in bytes)'], df['Cumulative RTT(in microseconds)'], label=f'TTL={ttl}', color=color)

plt.title('Cumulative RTT vs. Payload Size for Different TTL Values(Increased Traffic)')
plt.xlabel('Payload Size (bytes)')
plt.ylabel('Cumulative RTT (microseconds)')
plt.legend()
plt.grid(True)

plt.savefig('./TTL_Comparison_Increased Traffic.png')
plt.show()

