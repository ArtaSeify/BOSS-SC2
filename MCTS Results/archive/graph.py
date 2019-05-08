import numpy as np
import os
import pickle
import matplotlib.pyplot as plt

def drawGraph(values, name):
	# draw each line
	for run in range(len(values)):
		number_of_simulations = len(values[run])
		simulations = [10*i for i in range(int(number_of_simulations))]
		plt.plot(simulations, values[run][:int(number_of_simulations)], label="run: " + str(run))
	
	plt.legend(loc="upper left")
	plt.xlabel("Number of simulations", fontsize='xx-large')
	plt.ylabel("Integral value", fontsize='xx-large')
	# if searchLength == "10000":
	plt.gca().set_ylim([0, 300000])
	# elif searchLength == "8000":
	# 	plt.gca().set_ylim([0, 150000])
	# elif searchLength == "6000":
	# 	plt.gca().set_ylim([0, 50000])
	# elif searchLength == "4000":
	# 	plt.gca().set_ylim([0, 10000])
	#plt.show()
	
	plt.savefig(name)
	plt.clf()

data_names = ["CRMV", "CRAV", "SRAV", "SRMV", "SRAV2", "SRMV2"]

for data_name in data_names:
	data_dir = os.path.join(os.path.join(os.getcwd(), "parsed data"), data_name)
	graphs_dir = os.path.join(os.getcwd(), "graphs")
	saved_every_simulations = 10

	pickle_in = open(os.path.join(data_dir, "values.pickle"),"rb")
	values = pickle.load(pickle_in)

	plt.figure(figsize=(30.0, 18.0)) # in inches!	

	lowest_length = 9999999
	average = dict()
	std = dict()
	for cValue in values:
		# graph for every run
		save_dir = os.path.join(graphs_dir, "HyperParameters_" + data_name)
		if not os.path.isdir(save_dir):
			os.makedirs(save_dir)


		# make them all the same size
		run_lenghts = [ len(l) for l in values[cValue] ] 
		min_length = np.min(run_lenghts)
		lowest_length = min(min_length, lowest_length)
		for run in range(len(values[cValue])):
			values[cValue][run] = values[cValue][run][0:min_length]

		# average and std
		values[cValue] = np.array(values[cValue])
		average[cValue] = np.mean(values[cValue], axis=0)
		std[cValue] = np.std(values[cValue], axis=0)

		drawGraph(values[cValue], os.path.join(save_dir, "MCTS_" + cValue + "_highestValue_runs.png"))

	max_values = [ np.max(average[cValue][0:lowest_length]) for cValue in values]
	for ind,cValue in enumerate(values):
		print("data: " + data_name + ", exploration parameter: " + str(cValue) + ", highest value: " + str(max_values[ind]))

	import random
	# create a graph that contains all the average results
	for cValue in values:
		number_of_simulations = lowest_length
		simulations = [10*i for i in range(int(number_of_simulations))]
		errorevery = random.randint(800, 1501)

		average[cValue] = average[cValue][0:number_of_simulations]
		std[cValue] = std[cValue][0:number_of_simulations]

		plt.errorbar(simulations, average[cValue], std[cValue], 
					errorevery=errorevery, label=str(cValue))

	plt.legend(loc="upper left")
	plt.xlabel("Number of simulations", fontsize='xx-large')
	plt.ylabel("Integral value", fontsize='xx-large')
	# if searchLength == "10000":
	plt.gca().set_ylim([0, 300000])
	# elif searchLength == "8000":
	# 	plt.gca().set_ylim([0, 150000])
	# elif searchLength == "6000":
	# 	plt.gca().set_ylim([0, 50000])
	# elif searchLength == "4000":
	# 	plt.gca().set_ylim([0, 10000])
	#plt.show()

	save_dir = os.path.join(graphs_dir, "HyperParameters_" + data_name)
	if not os.path.isdir(save_dir):
		os.makedirs(save_dir)

	save_dir = os.path.join(save_dir, "average" + ".png")
	plt.savefig(save_dir)
	plt.clf()