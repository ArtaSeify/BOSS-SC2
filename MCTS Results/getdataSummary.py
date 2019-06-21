import numpy as np
import os
import pickle
import argparse
from math import ceil, sqrt
import matplotlib.pyplot as plt
import json

parser = argparse.ArgumentParser()
parser.add_argument("files_dir", help="Name of dir containing the data files")
parser.add_argument("save_dir", help="Name of save dir")
parser.add_argument("save_name", help="Name of file to be saved")
parser.add_argument("strings_in_data", help="The strings the data file name must include to be considered, seperated by commas")
parser.add_argument("--topk", help="Top k results to show")
args = parser.parse_args()

args.files_dir = os.path.join("parsed data", args.files_dir)
strings_in_data = args.strings_in_data.split(",")

data_files = os.listdir(args.files_dir)

all_data = []
for data_file in data_files:
	if any(s in data_file for s in strings_in_data):
		with open(os.path.join(args.files_dir, data_file), "r") as json_file:
			bestAvgValue = 0
			bestValue = 0
			avgAvgValue = 0
			avgSquaredValues = 0
			avgBest = 0
			bestSquaredValues = 0
			total_runs = 0
			#print(data_file)
			json_data = json_file.read()
			data = json.loads(json_data)
			for run in data:
				if len(data[run]) > 0:
					bestAvgValue = max(bestAvgValue, float(data[run][0]["Average"]["Eval"]))
					bestValue = max(bestValue, float(data[run][0]["Best"]["Eval"]))
					#print("run: " +str(run) + " " + str(data[run][0]["Best"]["UsefulEval"]))
					avgAvgValue += float(data[run][0]["Average"]["Eval"])
					avgSquaredValues += (float(data[run][0]["Average"]["Eval"]) * float(data[run][0]["Average"]["Eval"]))
					avgBest += float(data[run][0]["Best"]["Eval"])
					bestSquaredValues += (float(data[run][0]["Best"]["Eval"]) * float(data[run][0]["Best"]["Eval"]))
					total_runs += 1

			avgAvgValue /= total_runs
			avgBest /= total_runs
			SDAvg = sqrt((avgSquaredValues - (total_runs * avgAvgValue * avgAvgValue))/(total_runs - 1))
			SDBest = sqrt((bestSquaredValues - (total_runs * avgBest * avgBest))/(total_runs - 1))
			
			all_data.append((data_file, bestAvgValue, avgAvgValue, SDAvg, bestValue, avgBest, SDBest))
			#print("Data: {}. Highest value found: {}, Average value: {}, SD: {}".format(data_file, bestValue, avgValue, sqrt((squaredValues - (total_runs * avgValue * avgValue))/(total_runs - 1))))

all_data.sort(key=lambda x: x[5], reverse=True)

for d in all_data:
	print(d)