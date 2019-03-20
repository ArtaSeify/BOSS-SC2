import argparse
import os
from random import shuffle
from math import ceil, floor
#from data_filter_functions import parseLine, createUnitDict
from data_filter_functions import createUnitDict
from data_filter_all_functions import parseLine
import json

parser = argparse.ArgumentParser()
parser.add_argument("input_file_name", help="Name of input file to parse")
parser.add_argument("output_file_name", help="Name of parsed output file")
parser.add_argument("input_folder", help="path to directory containing data files")
parser.add_argument("output_folder", help="path to directory to put files in")
parser.add_argument("testset_size", help="size of the test set")
parser.add_argument("--move_name", help="Changes the name of file once finished")
parser.add_argument("--move_folder", help="Changes the folder of file once finished")
parser.add_argument("--delete_file", help="Delete the file once finished parsing")
args = parser.parse_args()

input_file_name = os.path.join(args.input_folder, args.input_file_name)
train_file_name = os.path.join(args.output_folder, args.output_file_name)
test_file_name = os.path.join(args.output_folder, "testset_" + args.output_file_name)
unit_dict = createUnitDict("C:\\School Work\\BOSS\\bin\\data\\ActionData.txt")
with open("C:\\School Work\\BOSS\\bin\\SC2Data.json") as sc2dataFile:
    sc2data = json.load(sc2dataFile)
    mins_per_worker_per_sec = sc2data["MineralsPerWorkerPerFrame"]
    gas_per_worker_per_sec = sc2data["GasPerWorkerPerFrame"]

os.chdir(args.input_folder)
if not os.path.isdir(os.path.join(os.getcwd(), args.output_folder)):
    os.makedirs(args.output_folder)

data_file = open(input_file_name, 'r')
trainset_file = open(train_file_name, 'w')
testset_file = open(test_file_name, 'w')

lines = data_file.readlines()
shuffle(lines)
testset = lines[0:floor(len(lines) * float(args.testset_size))]
trainset = lines[floor(len(lines) * float(args.testset_size)):]

MAX_NUM_UNITS = 0

for line in lines:
    units_finish = line.find("]]")
    units = line[line.find("[[")+1:units_finish+1]
    units = units.split("]")
    # remove the empty character at the end of the list
    units = units[: len(units)-1]
    unit_count = int(units[len(units)-1].split("[")[1].split(",")[0]) + 1

    if unit_count > MAX_NUM_UNITS:
        MAX_NUM_UNITS = unit_count

print(MAX_NUM_UNITS)

for index,line in enumerate(trainset):
    parseLine(line, unit_dict, mins_per_worker_per_sec, gas_per_worker_per_sec, MAX_NUM_UNITS, 0, trainset_file)

for index,line in enumerate(testset):
    parseLine(line, unit_dict, mins_per_worker_per_sec, gas_per_worker_per_sec, MAX_NUM_UNITS, 0, testset_file)

data_file.close()
trainset_file.close()
testset_file.close()

# delete file
if args.delete_file is not None:
    os.remove(input_file_name)

# move file
if args.move_name is not None and args.delete_file is None:
    if not os.path.isdir(os.path.join(os.getcwd(), args.move_folder)):
        os.makedirs(args.move_folder)
    os.rename(input_file_name, os.path.join(
                            os.path.join(os.getcwd(), args.move_folder),
                            args.move_name))