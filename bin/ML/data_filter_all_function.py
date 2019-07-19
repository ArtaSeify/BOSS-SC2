NUM_PROTOSS_ACTIONS = 70

#def createUnitDict(filename):
#    unit_info = dict()
#    with open(filename, 'r') as unit_file:
#        all_units = unit_file.readlines()
#        for unit in all_units:
#            unit = unit.split(",")
#            unit_info[unit[1].split("\n")[0]] = int(unit[0])

#    return unit_info

#def createActionVector(values):
#    action_probs = [0 for i in range(NUM_PROTOSS_ACTIONS)]
#    total = 0
#    for i in range(0,len(values)-1,2):
#        action_probs[int(values[i])] = 1
#        total += 1

#    action_probs = [i/total for i in action_probs]
#    return action_probs

def splitArrayData(data):
    data = data.split("]")
    # remove the empty character at the end of the list
    data = data[: len(data)-1]

    all_units = []
    for unit in data:
        unit = unit.split("[")[1]

        unit_types[int(unit.split(",")[4])] += 1
        
        all_units.append(unit)
        
    return all_units, unit_types

"""
    output_type: 0 = file
                 1 = string
"""
def parseLine(line, unit_dict, mins_per_worker_per_sec, gas_per_worker_per_sec,
                    MAX_NUM_UNITS, output_type, parsed_file=None):
    NUM_UNIT_FEATURES = 15

    units_finish = line.find("]]")
    units = line[line.find("[[")+1:units_finish+1]
    all_units, unit_types = splitArrayData(units)

    line = line[units_finish+3:]

    being_built_finish = line.find("]")
    being_built = line[line.find("[")+1:being_built_finish]
    being_built = being_built.split(",")

    line = line[being_built_finish+2:]

    finished_finish = line.find("]")
    finished = line[line.find("[")+1:finished_finish]
    finished = finished.split(",")

    line = line[finished_finish+2:]

    chronoboosts_finish = line.find("]]")
    chronoboosts = []
    if chronoboosts_finish != -1:
        chronoboosts = line[line.find("[[")+1:chronoboosts_finish+1]
        chronoboosts = splitArrayData(chronoboosts)
        print(chronoboosts)
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

    state_action_values = line[last_ability_finish+2:]
    if len(state_action_values) > 0:
        state_action_values = state_action_values.split(",")

        action_vector = createActionVector(state_action_values)
        state_value = state_action_values[-1]

    if output_type == 0:
        # unit info
        for unit_data in all_units:
            parsed_file.write(str(unit_data) + ",")

        empty_unit = ""
        for i in range(NUM_UNIT_FEATURES-1):
            empty_unit += "0,"
        empty_unit += "0"
        for units_missing in range(MAX_NUM_UNITS - len(all_units)):
            parsed_file.write(empty_unit + ",")

        for unit_type in unit_types:
            parsed_file.write(str(unit_type) + ",")

        # for i in being_built:
        #     print(i)
        #     parsed_file.write(str(i) + ",")

        # for i in finished:
        #     parsed_file.write(str(i) + ",")

        # for chronoBoost in chronoboosts:
        #     for value in chronoBoost:
        #         parsed_file.write(str(value) + ",")

        # state info
        parsed_file.write(race + ",")
        parsed_file.write(minerals + ",")
        parsed_file.write(gas + ",")
        parsed_file.write(current_supply + ",")
        parsed_file.write(max_supply + ",")
        parsed_file.write(current_frame + ",")
        parsed_file.write(str(int(frame_limit)-int(current_frame)) + ",")
        parsed_file.write(mineral_workers + ",")
        parsed_file.write(gas_workers + ",")
        parsed_file.write(building_workers + ",")
        parsed_file.write(str(mins_per_worker_per_sec) + ",")

        if len(state_action_values) == 0:
            parsed_file.write(str(gas_per_worker_per_sec))
        else:
            parsed_file.write(str(gas_per_worker_per_sec) + ",")

            for action_prob in action_vector:
                parsed_file.write(str(action_prob) + ",")
            # y value
            parsed_file.write(state_value)

    elif output_type == 1:
        output = ""
        # unit info
        for unit_data in all_units:
            output += (str(unit_data) + ",")

        empty_unit = ""
        for i in range(NUM_UNIT_FEATURES-1):
            empty_unit += "0,"
        empty_unit += "0"
        for units_missing in range(MAX_NUM_UNITS - len(all_units)):
            output += (empty_unit) + ","

        for unit_type in unit_types:
            output += (str(unit_type) + ",")

        # for i in being_built:
        #     print(i)
        #     output += str(i) + ","

        # for i in finished:
        #     output += str(i) + ","

        # for chronoBoost in chronoboosts:
        #     for value in chronoBoost:
        #         output += str(value) + ","

        # state info
        output += race + ","
        output += minerals + ","
        output += gas + ","
        output += current_supply + ","
        output += max_supply + ","
        output += current_frame + ","
        output += str(int(frame_limit)-int(current_frame)) + ","
        output += mineral_workers + ","
        output += gas_workers + ","
        output += building_workers + ","
        output += str(mins_per_worker_per_sec) + ","

        if len(state_action_values) == 0:
            output += str(gas_per_worker_per_sec)
        else:
            output += str(gas_per_worker_per_sec) + ","

            for action_prob in action_vector:
                output += (str(action_prob) + ",")
            # y value
            output += state_value

        return output