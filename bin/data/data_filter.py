import argparse
import os
from random import shuffle
from math import ceil
from data_filter_functions import parseLine, createUnitDict

parser = argparse.ArgumentParser()
parser.add_argument("input_file_name", help="Name of input file to parse")
parser.add_argument("output_file_name", help="Name of parsed output file")
parser.add_argument("input_folder", help="Name of directory containing data files")
parser.add_argument("output_folder", help="Folder to put files in")
parser.add_argument("--move_name", help="Changes the name of file once finished")
parser.add_argument("--move_folder", help="Changes the folder of file once finished")
parser.add_argument("--delete_file", help="Delete the file once finished parsing")
parser.add_argument("--shuffle", help="Whether to shuffle the lines")
args = parser.parse_args()

input_file_name = os.path.join(args.input_folder, args.input_file_name)
output_file_name = os.path.join(args.output_folder, args.output_file_name)
mins_per_worker_per_sec = "0.045"
gas_per_worker_per_sec = "0.07"
unit_dict = createUnitDict("C:\\School Work\\BOSS\\bin\\data\\ActionData.txt")

os.chdir("DataTuples")
if not os.path.isdir(os.path.join(os.getcwd(), args.output_folder)):
	os.makedirs(args.output_folder)

data_file = open(input_file_name, 'r')
parsed_file = open(output_file_name, 'w')

lines = data_file.readlines()
if args.shuffle:
	shuffle(lines)
	
for index,line in enumerate(lines):
	parseLine(line, unit_dict, mins_per_worker_per_sec, gas_per_worker_per_sec, 0, parsed_file)

data_file.close()
parsed_file.close()

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