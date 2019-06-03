import numpy as np
import pickle
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("files_dir", help="Name of dir containing the data files")
parser.add_argument("save_dir", help="Name of save dir")
args = parser.parse_args()

files_dir = args.files_dir
save_dir = os.path.join(os.path.join(os.getcwd(), "parsed data"), args.save_dir)
results = dict()

def parseDataLine(line):
	dic = dict()
	values = line.split(",")
	dic["NodesExpanded"] = int(values[0])
	dic["NodeVisits"] = int(values[1])
	dic["LeafNodesExpanded"] = int(values[2])
	dic["LeafNodeVisits"] = int(values[3])
	dic["SearchTimeSeconds"] = float(values[4])
	dic["NumSimulations"] = int(values[5])
	dic["BuildOrder"] = values[6]
	dic["SearchValue"] = float(values[7])
	dic["SearchIntegral"] = float(values[8])

	return dic

def parse(name_prepend, cwd):
	all_files = os.listdir(cwd)

	for file in all_files:
		file_path = os.path.join(cwd, file)

		if "Run" in file:
			run = file.split("Run")[1].split("\n")[0]
			# create the dict if it doesn't exist
			if name_prepend not in results:
				results[name_prepend] = dict()
			results[name_prepend][run] = []
			# get the data for that run
			data_files = os.listdir(file_path)
			for data_file in data_files:
				if "Results.csv" in data_file:
					with open(os.path.join(file_path, data_file), "r") as results_csv:
						for line in results_csv:
							results[name_prepend][run].append(parseDataLine(line))

		# recuse
		else:
			if os.path.isdir(file_path):
				name = name_prepend
				if name_prepend is not "":
					name += "_"
				name += file 
				parse(name, file_path)
			else:
				print("MISTAKE!!!")

parse("", files_dir)

if not os.path.isdir(save_dir):
	os.makedirs(save_dir)

for key in results:
	n = key
	if n == '':
		n = "0"
	with open(os.path.join(save_dir, n), "wb") as output_file:
		pickle.dump(results[key], output_file)