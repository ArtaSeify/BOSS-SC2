import numpy as np
import pickle
import os
flags = ["TrueFlag", "FalseFlag"]

original_dir = os.getcwd()

for flag in flags:
	data_dir = os.path.join(os.path.join(os.getcwd(), "small range"), flag)
	save_dir = os.path.join(os.getcwd(), "parsed data")

	os.chdir(data_dir)
	directories = os.listdir(os.getcwd())
	curr_path = os.getcwd()
	folder_names = dict()

	# get folder directories for each cValue
	for direc in directories:
		if "MCTS" not in direc:
			continue

		path_to_folder = os.path.join(curr_path, direc)
		cValue = direc.split("Flag")[1].split("Run")[0].split("C")
		searchLength = cValue[0]
		cValue = cValue[1]

		# cValue doesn't exist as key. create a dict with that cValue key
		if cValue not in folder_names:
			folder_names[cValue] = dict()

		# add searchlength under cValue
		if searchLength not in folder_names[cValue]:
			folder_names[cValue][searchLength] = [path_to_folder]
		else:
			folder_names[cValue][searchLength].append(path_to_folder)



	# get the experiment results
	

	mostVisited = dict()
	highestValue = dict()
	for cValue in folder_names:
		print("working on cValue: " + str(cValue))
		mostVisited[cValue] = dict()
		highestValue[cValue] = dict()
		
		for searchLength in folder_names[cValue]:
			print("working on search length: " + str(searchLength))
			mostVisited[cValue][searchLength] = [[] for i in range(len(folder_names[cValue][searchLength]))]
			highestValue[cValue][searchLength] = [[] for i in range(len(folder_names[cValue][searchLength]))]

			for run,file_path in enumerate(folder_names[cValue][searchLength]):
				os.chdir(file_path)
				subdir_files = os.listdir()
				for subdir_file in subdir_files:
					if "Results.csv" in subdir_file:
						with open(subdir_file, "r") as results_csv:
							for line in results_csv:
								both_results = line.split(",")
								mostVisited[cValue][searchLength][run].append(float(both_results[0]))
								highestValue[cValue][searchLength][run].append(float(both_results[1]))

	output_file = open(os.path.join(save_dir, flag + "_mostVisited.pickle"),"wb")
	pickle.dump(mostVisited, output_file)
	output_file.close()

	output_file = open(os.path.join(save_dir, flag + "_highestValue.pickle"),"wb")
	pickle.dump(highestValue, output_file)
	output_file.close()

	os.chdir(original_dir)