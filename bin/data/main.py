import tensorflow as tf
import os
import argparse
#from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor

BIN_PATH = "C:\\School Work\\BOSS\\bin"
DATA_PATH = BIN_PATH + "\\data\\DataTuples\\ParsedSearchData"
DATA_FILE = os.path.join(DATA_PATH, "ParsedSearchData.csv")

class Driver:
	def __init__(self, args):
		self.sess = tf.Session()
		tf.keras.backend.set_session(self.sess)
		self.args = args

		# constants
		self.num_runs = 1
		self.cpu_workers = 4
		
		self.feature_shape = 332
		self.prediction_shape = 1
		self.learning_rate = 1e-4
		self.epochs = 1
		self.verbose = 1
		self.batch_size = 4

		self.createNetwork()

	def createNetwork(self):
		self.network = model.IntegralValueNN(self.feature_shape, self.prediction_shape, self.args.save_name, 
											self.batch_size, self.learning_rate)

	def train(self):
		# creating iterators to go through the datasets using the dataset API
		dataset = DataLoader(self.feature_shape, self.prediction_shape, self.batch_size, self.cpu_workers)
		train_iterator = dataset.make_iterator(self.sess, [DATA_FILE])
		trainset_samples = sum(1 for line in open(DATA_FILE))

		print(trainset_samples)

		# train and evaluate
		self.network.train(train_iterator, self.epochs, floor(trainset_samples/self.batch_size), self.verbose)

	def start(self):
		for run in range(self.num_runs):
			os.chdir(BIN_PATH)
			os.system("BOSS_main.exe")

			os.chdir("data")
			os.system("python data_filter.py " + self.args.frame_limit + " SearchData.csv ParsedSearchData.csv SearchData ParsedSearchData --move_name=iteration_" + str(run) + ".csv --move_folder=UsedSearchData")

			self.train()

			# save data and change name of data file so we can start next round
			self.network.save(self.args.save_name + ".h5")
			os.rename(DATA_FILE, os.path.join(DATA_PATH, "iteration_" + str(run) + ".csv"))

def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("save_name", help="Name of model to save")
	parser.add_argument("frame_limit", help="Frame limit of search")
	parser.add_argument("--load_model", help="Name of model to load")
	args = parser.parse_args()

	driver = Driver(args)

	driver.start()

if __name__ == "__main__":
	main()