import numpy as np
import os
import pickle
import matplotlib.pyplot as plt

flags = ["TrueFlag", "FalseFlag"]

def drawGraph(values, name):
	# draw each line
	for run in range(len(values)):
		plt.plot(simulations, values[run][:int(number_of_simulations/saved_every_simulations)], label="run: " + str(run))
	
	plt.legend(loc="upper left")
	plt.gca().set_ylim([0, 300000])
	#plt.show()
	
	plt.savefig(name)
	plt.clf()

for flag in flags:
	data_dir = os.path.join(os.path.join(os.getcwd(), "parsed data"), "values")
	graphs_dir = os.path.join(os.getcwd(), "graphs")
	saved_every_simulations = 10
	number_of_simulations = 100000

	pickle_in = open(os.path.join(data_dir, flag + "_Values_highestValue.pickle"),"rb")
	highestValue = pickle.load(pickle_in)

	simulations = [saved_every_simulations*i for i in range(int(number_of_simulations/saved_every_simulations))]
	plt.figure(figsize=(30.0, 18.0)) # in inches!

	average_highestValue = dict()
	std_highestValue = dict()

	# graph for every run
	for value_type in highestValue:
		highestValue[value_type] = np.array(highestValue[value_type])

		save_dir = os.path.join(graphs_dir, "values")
		if not os.path.isdir(save_dir):
			os.makedirs(save_dir)

		drawGraph(highestValue[value_type], os.path.join(save_dir, "MCTS_" + value_type + "_" + flag + "_runs.png"))


	# average
	for value_type in highestValue:
		average_highestValue[value_type] = np.mean(highestValue[value_type], axis=0)
		std_highestValue[value_type] = np.std(highestValue[value_type], axis=0)

	# create a graph that contains all the average results
	for value_type in average_highestValue:
		errorevery = 1000 if value_type == "Average" else 750
		print(value_type + " " + str(errorevery))
		plt.errorbar(simulations, average_highestValue[value_type], std_highestValue[value_type], 
					errorevery=errorevery, label="value calculation: " + str(value_type))

		plt.legend(loc="upper left")

	plt.gca().set_ylim([0, 300000])
	#plt.show()

	save_dir = os.path.join(graphs_dir, "values")
	if not os.path.isdir(save_dir):
		os.makedirs(save_dir)

	save_dir = os.path.join(save_dir, "MCTS_" + flag + "_average.png")
	plt.savefig(save_dir)
	plt.clf()