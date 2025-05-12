import pandas as pd
import sys
import matplotlib.pyplot as plt

data = pd.read_csv(sys.argv[1], sep=", ", engine='python')

for mode in data.columns:
    fig, ax = plt.subplots(figsize=[15,3])
    plt.scatter(data.index.array, data[mode])
    plt.title(mode)
    plt.savefig(f"{mode}_graph.svg")