import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("runs_folder", help="folder containing the runs")
args = parser.parse_args()

current_dir = os.getcwd()
all_runs_dir = os.path.join(current_dir, args.runs_folder)
all_runs = os.listdir(all_runs_dir)

max_value_found = 0

os.chdir(all_runs_dir)
for fold in all_runs:
	os.chdir(fold)
	files = os.listdir()
	for f in files:
		if "SearchData" in f:
			with open(f, 'r') as dataFile:
				data = dataFile.readlines()
				val = float(data[0].split(":")[1].split("\n")[0][1:])
				#print(val)
				#print(data[1].split(":")[1].split("\n")[0][1:].split(" "))
				if val > max_value_found:
					max_value_found = val
					best_build_order = data[1].split(":")[1].split("\n")[0][1:].split(" ")
					best_folder_name = fold

	os.chdir(all_runs_dir)

print(max_value_found)
print(best_build_order)					
print(best_folder_name)

