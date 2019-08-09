import numpy as np
import os
import pickle
import argparse
from math import ceil
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument("files_dirs", help="Name of dirs containing the data files")
parser.add_argument("save_dir", help="Name of save dir")
parser.add_argument("save_name", help="Name of file to be saved")
parser.add_argument("strings_in_data", help="The strings the data file name must include to be considered, seperated by commas")
parser.add_argument("strings_not_in_data", help="The strings the data file must not include to be considered")
parser.add_argument("--topk", help="Top k results to show")
parser.add_argument("--xaxis", help="The data for X-axis")
args = parser.parse_args()

files_dirs = args.files_dirs.split(",")
strings_in_data = args.strings_in_data.split(",")
strings_not_in_data = args.strings_not_in_data.split(",")
xaxis = args.xaxis if args.xaxis is not None else "NodeVisits"
plt.figure(figsize=(18.5, 10))

def drawGraph(x, max_x, y, max_y, name):
    # draw graph
    plt.plot(x, y, label=name)
    
    plt.legend(loc="lower right")
    plt.gca().set_xlim([0, max(max_x, plt.gca().get_xlim()[1])])
    plt.gca().set_ylim([0, max(max_y, plt.gca().get_ylim()[1])])
    #plt.yticks(list(plt.yticks()[0]) + [max_y])
    #plt.show()
    
    #plt.savefig(name)
    #plt.clf()

def fixData (x, y, max_x, isNetwork):
    assert len(x) == len(y)
    if isNetwork:
        x_range = [i for i in range(max_x + 1)]
    else:
        x_range = [1000*i for i in range(ceil(max_x/1000.0) + 1)]

    all_runs_x = [x_range for i in range(len(x))]
    all_runs_y = [[] for i in range(len(y))]
    
    for run in range(len(x)):
        max_x_index = [-1 for i in range(len(x_range))]
        max_x_index[0] = 0

        run_x = x[run]
        run_y = y[run]

        for ind in range(1, len(run_x)):
            if isNetwork:
                max_x_index[run_x[ind]] = ind
            else:
                max_x_index[int(run_x[ind] / 1000) + 1] = ind

        for index,val in enumerate(max_x_index):
            if val == -1:
                assert index > 0
                max_x_index[index] = max_x_index[index-1]
            all_runs_y[run].append(int(run_y[max_x_index[index]]))

    average_x = np.around(np.mean(all_runs_x, axis=0, dtype=np.float32), decimals=2)
    average_y = np.around(np.mean(all_runs_y, axis=0, dtype=np.float32), decimals=2)
    min_y = np.amin(all_runs_y, axis=0)
    max_y = np.amax(all_runs_y, axis=0)
    sd_y = np.around(np.std(all_runs_y, axis=0, dtype=np.float32), decimals=2)

    return average_x, average_y, min_y, max_y, sd_y


for files_dir_name in files_dirs:
    files_dir = os.path.join("parsed data", files_dir_name)
    data_files = os.listdir(files_dir)

    max_simulations = 0
    max_value = 0
    for data_file in data_files:
        if any(s in data_file for s in strings_in_data) and not any(s in data_file for s in strings_not_in_data):
            with open(os.path.join(files_dir, data_file), "rb") as pickle_in:
                data = pickle.load(pickle_in)
                for run in data:
                    if len(data[run]) > 0:
                        max_simulations = max(max_simulations, data[run][-1][xaxis])
                        if data[run][-1]["SearchIntegral"] > max_value:
                            max_value = data[run][-1]["SearchIntegral"]
                            max_value_file = files_dir_name + "_" + data_file + ", run: " + str(run) if len(files_dirs) > 1 else data_file + ", run: " + str(run) 


    all_data = []
    for data_file in data_files:
        if any(s in data_file for s in strings_in_data) and not any(s in data_file for s in strings_not_in_data):
            with open(os.path.join(files_dir, data_file), "rb") as pickle_in:
                data = pickle.load(pickle_in)

                if len(data) == 0:
                    continue
                
                # incomplete data
                badData = False
                for run in data:
                    if int(run) > len(data):
                        badData = True
                if badData:
                    print("skipping: {}".format(data_file))
                    continue

                simulations = [[] for run in data]
                values = [[] for run in data]
                for run in data:
                    ind = int(run)
                    for data_point in data[run]:
                        simulations[ind].append(data_point[xaxis])
                        values[ind].append(data_point["SearchIntegral"])
                x, y, min_y, max_y, sd_y, = fixData(simulations, values, max_simulations, True if "Network" in data_file else False)
                all_data.append((x, y, min_y, max_y, sd_y, files_dir_name + "_" + data_file if len(files_dir) > 0 else data_file))
                print("{}: Average: {:.2f}, Min-Max: {:.2f}-{:.2f}, SD: {:.2f}".format(data_file, y[-1], min_y[-1], max_y[-1], sd_y[-1]))


    all_data = sorted(all_data, reverse=True, key=lambda x: x[1][-1])

    if args.topk is None:
        topk = len(all_data)
        all_data.sort(key=lambda x: x[5])
    else:
        topk = args.topk
        all_data.sort(key=lambda x:x[1][-1], reverse=True)

    # all_data.sort(key=lambda x: int(x[2].split("_")[-1]))
    highestValues = []
    for i in range(int(topk)):
        print("{}: File: {}, Average: {:.2f}, Min-Max: {:.2f}-{:.2f}, SD: {:.2f}".format(
                str(i + 1), all_data[i][5], all_data[i][1][-1], all_data[i][2][-1], all_data[i][3][-1], all_data[i][4][-1]))
        highestValues.append(all_data[i][1][-1])
        #drawGraph(all_data[i][0], max_simulations, all_data[i][1], max_value, all_data[i][5])
    drawGraph(range(int(topk)), topk-1, highestValues, (5/4)*max(highestValues), files_dir_name)
    print("Highest value found: {:.2f}, in {}".format(max_value, max_value_file))

if not os.path.isdir(args.save_dir):
    os.makedirs(args.save_dir)

plt.savefig(os.path.join(args.save_dir, args.save_name + ".png"), format="png")