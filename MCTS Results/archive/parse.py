import numpy as np
import pickle
import os

#flags = ["TrueFlag", "FalseFlag"]
data_files_folder = "SRMV2"

original_dir = os.getcwd()

#for flag in flags:
data_dir = os.path.join(os.getcwd(), data_files_folder)
save_dir = os.path.join(os.path.join(os.getcwd(), "parsed data"), data_files_folder)

if not os.path.isdir(data_dir):
	print("data directory doesn't exist!")

if not os.path.isdir(save_dir):
	os.makedirs(save_dir)

os.chdir(data_dir)
directories = os.listdir(os.getcwd())
curr_path = os.getcwd()
folder_names = dict()

# get folder directories for each cValue
for direc in directories:
	#if "MCTS" not in direc:
		#continue

	path_to_folder = os.path.join(curr_path, direc)
	cValue = direc.split("Run")[0].split("C")[1]
	#cValue = cValue[1]

	# cValue doesn't exist as key. create a dict with that cValue key
	if cValue not in folder_names:
		folder_names[cValue] = []
	
	folder_names[cValue].append(path_to_folder)

		# add searchlength under cValue
		#if searchLength not in folder_names[cValue]:
			#folder_names[cValue][searchLength] = [path_to_folder]
		#else:
			#folder_names[cValue][searchLength].append(path_to_folder)

	# get the experiment results
	
highestValue = dict()
for cValue in folder_names:
	print("working on cValue: " + str(cValue))
	highestValue[cValue] = []
	
	#for searchLength in folder_names[cValue]:
		#print("working on search length: " + str(searchLength))
		#mostVisited[cValue][searchLength] = [[searchLength]))]
		#highestValue[cValue][searchLength] = [[] for i in range(len(folder_names[cValue][searchLength]))]

	for run,file_path in enumerate(folder_names[cValue]):
		os.chdir(file_path)
		subdir_files = os.listdir()
		for subdir_file in subdir_files:
			if "Results.csv" in subdir_file:
				with open(subdir_file, "r") as results_csv:
					all_data = results_csv.readline()
					all_data = all_data.split(",")
					del all_data[-1]
					all_data = [float(x) for x in all_data]
					highestValue[cValue].append(all_data)

output_file = open(os.path.join(save_dir, "values.pickle"),"wb")
pickle.dump(highestValue, output_file)
output_file.close()

os.chdir(original_dir)