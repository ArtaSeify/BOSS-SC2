import numpy as np
import os
import pickle
import argparse
from math import ceil
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument("files_dir", help="Name of dir containing the data files")
parser.add_argument("save_dir", help="Name of save dir")
parser.add_argument("save_name", help="Name of file to be saved")
parser.add_argument("strings_in_data", help="The strings the data file name must include to be considered, seperated by commas")
parser.add_argument("--topk", help="Top k results to show")
args = parser.parse_args()

strings_in_data = args.strings_in_data.split(",")
plt.figure(figsize=(18.5, 10))

def drawGraph(x, max_x, y, max_y, name):
	# draw graph
	plt.plot(x, y, label=name)
	
	plt.legend(loc="upper left")
	plt.gca().set_xlim([0, max_x])
	plt.gca().set_ylim([0, max_y])
	#plt.show()
	
	#plt.savefig(name)
	#plt.clf()

def fixData (x, y, max_x):
	assert len(x) == len(y)
	x_range = [1000*i for i in range(ceil(max_x/1000.0) + 1)]

	all_runs_x = [x_range for i in range(len(x))]
	all_runs_y = [[] for i in range(len(y))]
	
	for run in range(len(x)):
		max_x_index = [-1 for i in range(len(x_range))]
		max_x_index[0] = 0

		run_x = x[run]
		run_y = y[run]

		for ind in range(1, len(run_x)):
			max_x_index[int(run_x[ind] / 1000) + 1] = ind

		for index,val in enumerate(max_x_index):
			if val == -1:
				assert index > 0
				max_x_index[index] = max_x_index[index-1]
			all_runs_y[run].append(int(run_y[max_x_index[index]]))

	average_x = np.mean(all_runs_x, axis=0)
	average_y = np.mean(all_runs_y, axis=0)

	return average_x, average_y

data_files = os.listdir(args.files_dir)

max_simulations = 0
max_value = 0
for data_file in data_files:
	if all(s in data_file for s in strings_in_data):
		with open(os.path.join(args.files_dir, data_file), "rb") as pickle_in:
			data = pickle.load(pickle_in)
			for run in data:
				max_simulations = max(max_simulations, data[run][-1]["NumSimulations"])
				max_value = max(max_value, data[run][-1]["SearchIntegral"])


all_data = []
for data_file in data_files:
	if all(s in data_file for s in strings_in_data):
		with open(os.path.join(args.files_dir, data_file), "rb") as pickle_in:
			data = pickle.load(pickle_in)

			simulations = [[] for run in data]
			values = [[] for run in data]
			for run in data:
				ind = int(run)
				for data_point in data[run]:
					simulations[ind].append(data_point["NumSimulations"])
					values[ind].append(data_point["SearchIntegral"])
			x, y = fixData(simulations, values, max_simulations)
			all_data.append((x, y, data_file))
			print(data_file + " " + str(y[-1]))


all_data = sorted(all_data, reverse=True, key=lambda x: x[1][-1])
topk = len(all_data) if args.topk is None else args.topk

for i in range(int(topk)):
	print(str(i + 1) + ": " + str(all_data[i][1]) + " " + all_data[i][2])
	drawGraph(all_data[i][0], max_simulations, all_data[i][1], max_value, all_data[i][2])

if not os.path.isdir(args.save_dir):
	os.makedirs(args.save_dir)

plt.savefig(os.path.join(args.save_dir, args.save_name + ".png"), format="png")