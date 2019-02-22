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


"""
	output_type: 0 = file
				 1 = string
"""
def parseLine(line, unit_dict, mins_per_worker_per_sec, gas_per_worker_per_sec,
					output_type, parsed_file=None):
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
	frame_limit = single_values[7]
	mineral_workers = single_values[8]
	gas_workers = single_values[9]
	building_workers = single_values[10]
	num_refineries = single_values[11]
	num_depots = single_values[12]
	last_action = single_values[13]

	last_ability_finish = line.find("]")
	last_ability = line[line.find("[")+1:last_ability_finish]

	y_value = line[last_ability_finish+2:]

	if output_type == 0:
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

		if y_value == "":
			parsed_file.write(gas_per_worker_per_sec)
		else:
			parsed_file.write(gas_per_worker_per_sec + ",")

			# y value
			parsed_file.write(y_value)

	elif output_type == 1:
		output = ""

		# unit info
		for unit_list in unit_info:
			for value in unit_list:
				output += (str(value) + ",")
		output += (str(canChronoBoost) + ",")
		# state info
		output += (minerals + ",")
		output += (gas + ",")
		output += (current_supply + ",")
		output += (max_supply + ",")
		output += (current_frame + ",")
		output += (mineral_workers + ",")
		output += (gas_workers + ",")
		output += (frame_limit + ",")
		output += (mins_per_worker_per_sec + ",")

		if y_value == "":
			output += (gas_per_worker_per_sec)
		else:
			output += (gas_per_worker_per_sec + ",")

			# y value
			output += (y_value)

		return output