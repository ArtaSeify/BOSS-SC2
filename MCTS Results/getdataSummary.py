import numpy as np
import os
import pickle
import argparse
from math import ceil
import matplotlib.pyplot as plt
import json

parser = argparse.ArgumentParser()
parser.add_argument("files_dir", help="Name of dir containing the data files")
parser.add_argument("save_dir", help="Name of save dir")
parser.add_argument("save_name", help="Name of file to be saved")
parser.add_argument("strings_in_data", help="The strings the data file name must include to be considered, seperated by commas")
parser.add_argument("--topk", help="Top k results to show")
args = parser.parse_args()

strings_in_data = args.strings_in_data.split(",")

data_files = os.listdir(args.files_dir)

nodeVisits = 0
total_runs = 0
nodesPerSecond = 0
CPUTime = 0
for data_file in data_files:
	if all(s in data_file for s in strings_in_data):
		with open(os.path.join(args.files_dir, data_file), "r") as json_file:
			json_data = json_file.read()
			data = json.loads(json_data)
			for run in data:
				if len(data[run]) > 0:
					nodeVisits = int(data[run][0]["Average"]["NodeVisits"])
					CPUTime = float(data[run][0]["Average"]["TimeElapsedCPU"])
					nodesPerSecond += nodeVisits/CPUTime
					#print(data_file + " " + str(nodeVisits/CPUTime) + " " + str(nodeVisits))
					total_runs += 1

nodesPerSecond /= total_runs
print("Nodes Per Second: {}, Total Runs: {}".format(nodesPerSecond, total_runs))