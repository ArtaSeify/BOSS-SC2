import numpy as np
import pickle
import os

flags = ["TrueFlag", "FalseFlag"]
data_files_folder = "values"

original_dir = os.getcwd()

for flag in flags:
	data_dir = os.path.join(os.path.join(os.getcwd(), data_files_folder), flag)
	save_dir = os.path.join(os.path.join(os.getcwd(), "parsed data"), data_files_folder)

	if not os.path.isdir(data_dir):
		print("data directory doesn't exist!")
		break

	if not os.path.isdir(save_dir):
		os.makedirs(save_dir)

	os.chdir(data_dir)
	value_types_folders = os.listdir(os.getcwd())
	curr_path = os.getcwd()
	folder_names = dict()

	for value_type_folder in value_types_folders:
		os.chdir(value_type_folder)
		directories = os.listdir(os.getcwd())

		for direc in directories:
			if "MCTS" not in direc:
				continue

			path_to_folder = os.path.join(os.path.join(curr_path, value_type_folder), direc)
			value_type = value_type_folder

			# cValue doesn't exist as key. create a dict with that cValue key
			if value_type not in folder_names:
				folder_names[value_type] = []

			# add searchlength under cValue
			folder_names[value_type].append(path_to_folder)

		os.chdir("..")
	# get the experiment results
	

	highestValue = dict()
	for value_type in folder_names:
		highestValue[value_type] = [[] for i in range(len(folder_names[value_type]))]

		for run,file_path in enumerate(folder_names[value_type]):
			os.chdir(file_path)
			subdir_files = os.listdir()
			for subdir_file in subdir_files:
				if "Results.csv" in subdir_file:
					with open(subdir_file, "r") as results_csv:
						for line in results_csv:
							highestValue[value_type][run].append(float(line.split(",")[1]))

	output_file = open(os.path.join(save_dir, flag + "_Values" + "_highestValue.pickle"),"wb")
	pickle.dump(highestValue, output_file)
	output_file.close()

	os.chdir(original_dir)