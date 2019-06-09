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
parser.add_argument("strings_not_in_data", help="The strings the data file must not include to be considered")
parser.add_argument("--topk", help="Top k results to show")
args = parser.parse_args()

strings_in_data = args.strings_in_data.split(",")
strings_not_in_data = args.strings_not_in_data.split(",")
plt.figure(figsize=(18.5, 10))

def drawGraph(x, max_x, y, max_y, name):
	# draw graph
	plt.plot(x, y, label=name)
	
	plt.legend(loc="lower right")
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
	sd_y = np.std(all_runs_y, axis=0)

	return average_x, average_y, sd_y

data_files = os.listdir(args.files_dir)

max_simulations = 0
max_value = 0
for data_file in data_files:
	if any(s in data_file for s in strings_in_data) and not any(s in data_file for s in strings_not_in_data):
		with open(os.path.join(args.files_dir, data_file), "rb") as pickle_in:
			data = pickle.load(pickle_in)
			for run in data:
				max_simulations = max(max_simulations, data[run][-1]["NodeVisits"])
				max_value = max(max_value, data[run][-1]["SearchIntegral"])


all_data = []
for data_file in data_files:
	if any(s in data_file for s in strings_in_data) and not any(s in data_file for s in strings_not_in_data):
		with open(os.path.join(args.files_dir, data_file), "rb") as pickle_in:
			data = pickle.load(pickle_in)

			simulations = [[] for run in data]
			values = [[] for run in data]
			for run in data:
				ind = int(run)
				for data_point in data[run]:
					simulations[ind].append(data_point["NodeVisits"])
					values[ind].append(data_point["SearchIntegral"])
			x, y, sd_y = fixData(simulations, values, max_simulations)
			all_data.append((x, y, sd_y, data_file))
			print("{}: Average: {}, SD: {}".format(data_file, y[-1], sd_y[-1]))


all_data = sorted(all_data, reverse=True, key=lambda x: x[1][-1])

if args.topk is None:
	topk = len(all_data)
	all_data.sort(key=lambda x: x[3])
else:
	topk = args.topk
	all_data.sort(key=lambda x:x[1][-1], reverse=True)

# all_data.sort(key=lambda x: int(x[2].split("_")[-1]))
highestValues = []
for i in range(int(topk)):
	print("{}: File: {}, Average: {}, SD: {}".format(str(i + 1), all_data[i][3], all_data[i][1][-1], all_data[i][2][-1]))
	highestValues.append(all_data[i][1][-1])
	#drawGraph(all_data[i][0], max_simulations, all_data[i][1], max_value, all_data[i][2])
drawGraph(range(int(topk)), topk-1, highestValues, (5/4)*max(highestValues), "ExIt")
print("Highest value found in any search: {}".format(max_value))

if not os.path.isdir(args.save_dir):
	os.makedirs(args.save_dir)

plt.savefig(os.path.join(args.save_dir, args.save_name + ".png"), format="png")