import argparse
import os
from random import shuffle
from math import ceil

def createUnitDict(filename):
	unit_info = dict()
	with open(filename, 'r') as unit_file:
		all_units = unit_file.readlines()
		for unit in all_units:
			unit = unit.split(",")
			unit_info[unit[1].split("\n")[0]] = int(unit[0])

	return unit_info

def getUnitData(units, unit_info):
	unit_list = units.split("]")
	# remove the empty character at the end of the list
	unit_list = unit_list[0 : len(unit_list)-1]

	unit_types = [0 for i in range(len(unit_info))]
	units_free = [0 for i in range(len(unit_info))]
	units_being_built = [0 for i in range(len(unit_info))]
	canChronoBoost = 0

	for unit in unit_list:
		unit = unit.split("[")[1]
		csv_unit = unit.split(",")
		
		unit_id = int(csv_unit[0])
		frame_started = int(csv_unit[1])
		frame_finished = int(csv_unit[2])
		builder_id = int(csv_unit[3])
		type_id = int(csv_unit[4])
		addon_id = int(csv_unit[5])
		buildtype_id = int(csv_unit[6])
		build_id = int(csv_unit[7])
		job_id = int(csv_unit[8])
		time_until_built = int(csv_unit[9])
		time_until_free = int(csv_unit[10])
		time_chronoboost = int(csv_unit[11])
		time_chronoboost_again = int(csv_unit[12])
		max_energy = int(csv_unit[13])
		energy = float(csv_unit[14])

		# do stuff with the data here
		unit_types[type_id] += 1
		if max(time_until_built, time_until_free) == 0:
			units_free[type_id] += 1
		if time_until_built > 0:
			units_being_built[type_id] += 1

		# Nexus can use chronoboost
		if type_id == unit_info["Nexus"] and energy >= 50.0:
			canChronoBoost = 1

	return [unit_types, units_free, units_being_built], canChronoBoost

parser = argparse.ArgumentParser()
parser.add_argument("frame_limit", help="The search frame limit")
parser.add_argument("input_file_name", help="Name of input file to parse")
parser.add_argument("output_file_name", help="Name of parsed output file")
parser.add_argument("input_folder", help="Name of directory containing data files")
parser.add_argument("output_folder", help="Folder to put files in")
parser.add_argument("--move_name", help="Changes the name of file once finished")
parser.add_argument("--move_folder", help="Changes the folder of file once finished")
args = parser.parse_args()

frame_limit = args.frame_limit

input_file_name = os.path.join(os.path.join(os.path.join(os.getcwd(), "DataTuples"), args.input_folder), args.input_file_name)
output_file_name = os.path.join(os.path.join(os.path.join(os.getcwd(), "DataTuples"), args.output_folder), args.output_file_name)
mins_per_worker_per_sec = "0.045"
gas_per_worker_per_sec = "0.07"
unit_dict = createUnitDict("ActionData.txt")

os.chdir("DataTuples")
if not os.path.isdir(os.path.join(os.getcwd(), args.output_folder)):
	os.makedirs(args.output_folder)

data_file = open(input_file_name, 'r')
# write to new, formatted csv file
parsed_file = open(output_file_name, 'w')

lines = data_file.readlines()
shuffle(lines)

for index,line in enumerate(lines):
	if index % 50000 == 0 and index > 0:
		print("finished: " + str(index) + " lines out of " + str(len(lines)))

	units_finish = line.find("]]")
	units = line[line.find("[[")+1:units_finish+1]
	unit_info, canChronoBoost = getUnitData(units, unit_dict)

	line = line[units_finish+3:]

	being_built_finish = line.find("]")
	being_built = line[line.find("[")+1:being_built_finish]

	line = line[being_built_finish+2:]

	finished_finish = line.find("]")
	finished = line[line.find("[")+1:finished_finish]

	line = line[finished_finish+2:]

	chronoboosts_finish = line.find("]]")
	if chronoboosts_finish != -1:
		chronoboosts = line[line.find("[[")+1:chronoboosts_finish+1]
		line = line[chronoboosts_finish+3:]

	else:
		line = line[3:]

	single_values = line.split(",")
	race = single_values[0]
	minerals = single_values[1]
	gas = single_values[2]
	current_supply = single_values[3]
	max_supply = single_values[4]
	current_frame = single_values[5]
	previous_frame = single_values[6]
	mineral_workers = single_values[7]
	gas_workers = single_values[8]
	building_workers = single_values[9]
	num_refineries = single_values[10]
	num_depots = single_values[11]
	last_action = single_values[12]

	last_ability_finish = line.find("]")
	last_ability = line[line.find("[")+1:last_ability_finish]

	y_value = line[last_ability_finish+2:]

	# unit info
	for unit_list in unit_info:
		for value in unit_list:
			parsed_file.write(str(value) + ",")
	parsed_file.write(str(canChronoBoost) + ",")

	# state info
	parsed_file.write(minerals + ",")
	parsed_file.write(gas + ",")
	parsed_file.write(current_supply + ",")
	parsed_file.write(max_supply + ",")
	parsed_file.write(current_frame + ",")
	parsed_file.write(mineral_workers + ",")
	parsed_file.write(gas_workers + ",")
	parsed_file.write(frame_limit + ",")
	parsed_file.write(mins_per_worker_per_sec + ",")
	parsed_file.write(gas_per_worker_per_sec + ",")

	# y value
	parsed_file.write(y_value)

data_file.close()
parsed_file.close()

# move file
print(args.move_name)
if args.move_name is not None:
	if not os.path.isdir(os.path.join(os.getcwd(), args.move_folder)):
		os.makedirs(args.move_folder)
	os.rename(input_file_name, os.path.join(
							os.path.join(os.getcwd(), args.move_folder),
							args.move_name))