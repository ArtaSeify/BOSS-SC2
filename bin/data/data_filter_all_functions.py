def createUnitDict(filename):
    unit_info = dict()
    with open(filename, 'r') as unit_file:
        all_units = unit_file.readlines()
        for unit in all_units:
            unit = unit.split(",")
            unit_info[unit[1].split("\n")[0]] = int(unit[0])

    return unit_info

def splitArrayData(data):
    #NUM_PROTOSS_ACTIONS = 68;

    data = data.split("]")
    # remove the empty character at the end of the list
    data = data[: len(data)-1]

    all_units = []
    for unit in data:
        unit = unit.split("[")[1]
        csv_unit = unit.split(",")
        
        all_units.append(csv_unit)
        
    return all_units

"""
    output_type: 0 = file
                 1 = string
"""
def parseLine(line, unit_dict, mins_per_worker_per_sec, gas_per_worker_per_sec,
                    MAX_NUM_UNITS, output_type, parsed_file=None):
    units_finish = line.find("]]")
    units = line[line.find("[[")+1:units_finish+1]
    all_units = splitArrayData(units)

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

    y_value = line[last_ability_finish+2:]

    if output_type == 0:
        # unit info
        for unit in all_units:
            for value in unit:
                parsed_file.write(str(value) + ",")
        for units_missing in range(MAX_NUM_UNITS - len(all_units)):
            for value_missing in range(len(all_units[0])):
                parsed_file.write("0,")

        # for i in being_built:
        #     print(i)
        #     parsed_file.write(str(i) + ",")

        # for i in finished:
        #     parsed_file.write(str(i) + ",")

        for chronoBoost in chronoboosts:
            for value in chronoBoost:
                parsed_file.write(str(value) + ",")

        # state info
        parsed_file.write(race + ",")
        parsed_file.write(minerals + ",")
        parsed_file.write(gas + ",")
        parsed_file.write(current_supply + ",")
        parsed_file.write(max_supply + ",")
        parsed_file.write(current_frame + ",")
        parsed_file.write(previous_frame + ",")
        parsed_file.write(frame_limit + ",")
        parsed_file.write(mineral_workers + ",")
        parsed_file.write(gas_workers + ",")
        parsed_file.write(building_workers + ",")
        parsed_file.write(str(mins_per_worker_per_sec) + ",")

        if y_value == "":
            parsed_file.write(str(gas_per_worker_per_sec))
        else:
            parsed_file.write(str(gas_per_worker_per_sec) + ",")

            # y value
            parsed_file.write(y_value)

    elif output_type == 1:
        output = ""
        # unit info
        for unit in all_units:
            for value in unit:
                output += str(value) + ","
        for units_missing in range(MAX_NUM_UNITS - len(all_units)):
            for value_missing in range(len(all_units[0])):
                output += "0,"

        # for i in being_built:
        #     print(i)
        #     output += str(i) + ","

        # for i in finished:
        #     output += str(i) + ","

        for chronoBoost in chronoboosts:
            for value in chronoBoost:
                output += str(value) + ","

        # state info
        output += race + ","
        output += minerals + ","
        output += gas + ","
        output += current_supply + ","
        output += max_supply + ","
        output += current_frame + ","
        output += previous_frame + ","
        output += frame_limit + ","
        output += mineral_workers + ","
        output += gas_workers + ","
        output += building_workers + ","
        output += str(mins_per_worker_per_sec) + ","

        if y_value == "":
            output += str(gas_per_worker_per_sec)
        else:
            output += str(gas_per_worker_per_sec) + ","

            # y value
            output += y_value

        return output