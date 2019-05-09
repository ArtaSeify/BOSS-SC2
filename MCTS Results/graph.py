import numpy as np
import os
import pickle
import argparse
from math import ceil
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument("files_dir", help="Name of dir containing the data files")
parser.add_argument("save_dir", help="Name of save dir")
parser.add_argument("name_in_data", help="The name the data file must include to be considered")
args = parser.parse_args()

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
	if args.name_in_data in data_file:
		with open(os.path.join(args.files_dir, data_file), "rb") as pickle_in:
			data = pickle.load(pickle_in)
			for run in data:
				max_simulations = max(max_simulations, data[run][-1]["NumSimulations"])
				max_value = max(max_value, data[run][-1]["SearchEval"])

for data_file in data_files:
	if args.name_in_data in data_file:
		with open(os.path.join(args.files_dir, data_file), "rb") as pickle_in:
			data = pickle.load(pickle_in)

			simulations = [[] for run in data]
			values = [[] for run in data]
			for run in data:
				ind = int(run)
				for data_point in data[run]:
					simulations[ind].append(data_point["NumSimulations"])
					values[ind].append(data_point["SearchEval"])
			x, y = fixData(simulations, values, max_simulations)
			drawGraph(x, max_simulations, y, max_value, data_file)

plt.show()