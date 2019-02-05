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

	if searchLength == "10000":
		plt.gca().set_ylim([0, 250000])
	elif searchLength == "8000":
		plt.gca().set_ylim([0, 150000])
	elif searchLength == "6000":
		plt.gca().set_ylim([0, 50000])
	elif searchLength == "4000":
		plt.gca().set_ylim([0, 10000])
	#plt.show()
	
	plt.savefig(name)
	plt.clf()

for flag in flags:
	data_dir = os.path.join(os.getcwd(), "parsed data")
	graphs_dir = os.path.join(os.getcwd(), "graphs")
	saved_every_simulations = 10
	number_of_simulations = 100000

	pickle_in = open(os.path.join(data_dir, flag + "_mostVisited.pickle"),"rb")
	mostVisited = pickle.load(pickle_in)
	pickle_in = open(os.path.join(data_dir, flag + "_highestValue.pickle"),"rb")
	highestValue = pickle.load(pickle_in)


	simulations = [saved_every_simulations*i for i in range(int(number_of_simulations/saved_every_simulations))]
	plt.figure(figsize=(30.0, 18.0)) # in inches!

	average_mostVisited = dict()
	average_highestValue = dict()

	# graph for every run
	for cValue in mostVisited:
		for searchLength in mostVisited[cValue]:
			mostVisited[cValue][searchLength] = np.array(mostVisited[cValue][searchLength])
			highestValue[cValue][searchLength] = np.array(highestValue[cValue][searchLength])

			save_dir = os.path.join(graphs_dir, flag)
			if not os.path.isdir(save_dir):
				os.makedirs(save_dir)

			save_dir = os.path.join(save_dir, searchLength)
			if not os.path.isdir(save_dir):
				os.makedirs(save_dir)

			drawGraph(mostVisited[cValue][searchLength], os.path.join(save_dir, "MCTS_" + cValue + "_" + searchLength + "_mostVisited_runs.png"))
			drawGraph(highestValue[cValue][searchLength], os.path.join(save_dir, "MCTS_" + cValue + "_" + searchLength + "_highestValue_runs.png"))


	# average
	for cValue in mostVisited:
		average_mostVisited[cValue] = dict()
		average_highestValue[cValue] = dict()

		for searchLength in mostVisited[cValue]:
			average_mostVisited[cValue][searchLength] = np.mean(mostVisited[cValue][searchLength], axis=0)
			average_highestValue[cValue][searchLength] = np.mean(highestValue[cValue][searchLength], axis=0)


	allcValues = list(average_mostVisited.keys())
	allLengths = list(average_mostVisited[allcValues[0]].keys())

	# create a graph that contains all the average results
	for searchLength in allLengths:
		for cValue in allcValues:
			linestyle = 'solid'

			plt.plot(simulations, average_mostVisited[cValue][searchLength][:int(number_of_simulations/saved_every_simulations)], linestyle=linestyle, label="mostVisited with cValue: " + str(cValue))
			plt.plot(simulations, average_highestValue[cValue][searchLength][:int(number_of_simulations/saved_every_simulations)], linestyle=linestyle, label="highestValue with cValue: " + str(cValue))

		plt.legend(loc="upper left")

		if searchLength == "10000":
			plt.gca().set_ylim([0, 250000])
		elif searchLength == "8000":
			plt.gca().set_ylim([0, 150000])
		elif searchLength == "6000":
			plt.gca().set_ylim([0, 50000])
		elif searchLength == "4000":
			plt.gca().set_ylim([0, 10000])
		#plt.show()

		save_dir = os.path.join(graphs_dir, flag)
		if not os.path.isdir(save_dir):
			os.makedirs(save_dir)

		save_dir = os.path.join(save_dir, "average")
		if not os.path.isdir(save_dir):
			os.makedirs(save_dir)

		save_dir = os.path.join(save_dir, "MCTS_" + searchLength + ".png")
		plt.savefig(save_dir)
		plt.clf()