import matplotlib.pyplot as plt
import os

all_files = os.listdir()
all_data = dict()
for f_name in all_files:
	if ".txt" in f_name:
		with open(f_name, 'r') as f:
			all_data[f_name.split(".")[0]] = list(map(float, f.read().splitlines()))


for unit_name in all_data:
	plt.clf()
	plt.hist(all_data[unit_name], bins=250)
	plt.savefig(unit_name + ".png")
