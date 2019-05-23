import tensorflow as tf
import os
import argparse
from data_loader import DataLoader
import model
from math import floor
import shutil

BIN_PATH = "C:\\School Work\\BOSS\\bin"
PARSER_PATH = BIN_PATH + "\\data"
MODELS_PATH = PARSER_PATH + "\\models"
DATA_PATH = BIN_PATH + "\\data\\DataTuples\\ParsedSearchData"
DATA_FILE = os.path.join(DATA_PATH, "ParsedSearchData.csv")

driver = None

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
		self.epochs = 10
		self.verbose = 1
		self.batch_size = 16

		self.createNetwork()
		self.network.save(MODELS_PATH + "\\" + self.args.save_name + ".h5")
		
		if self.args.use_predictions:
			tf.keras.backend.clear_session()

	def createNetwork(self):
		self.network = model.IntegralValueNN(self.feature_shape, self.prediction_shape, self.args.save_name, 
											self.batch_size, self.learning_rate)

	def train(self):
		# creating iterators to go through the datasets using the dataset API
		dataset = DataLoader(self.feature_shape, self.prediction_shape, True, self.batch_size, self.cpu_workers)
		train_iterator = dataset.make_iterator(self.sess, [DATA_FILE])
		trainset_samples = sum(1 for line in open(DATA_FILE))

		# train and evaluate
		self.network.train(train_iterator, self.epochs, floor(trainset_samples/self.batch_size), self.verbose)

	def start(self):
		for run in range(self.num_runs):
			if self.args.use_predictions:
				os.system("\"" + BIN_PATH + "\\BOSS_main.exe\" " + self.args.save_name)
			else:
				os.system("\"" + BIN_PATH + "\\BOSS_main.exe\"")

			os.chdir(BIN_PATH + "\\data")
			os.system("python data_filter.py SearchData.csv ParsedSearchData.csv SearchData ParsedSearchData --move_name=iteration_" + str(run) + ".csv --move_folder=UsedSearchData")

			if self.args.use_predictions:
				self.sess = tf.Session()
				tf.keras.backend.set_session(self.sess)
				self.createNetwork()

			self.train()
			self.network.save(MODELS_PATH + "\\" + self.args.save_name + ".h5")

			if self.args.use_predictions:
				tf.keras.backend.clear_session()

			os.rename(DATA_FILE, os.path.join(DATA_PATH, "iteration_" + str(run) + ".csv"))

def main():
	global driver

	parser = argparse.ArgumentParser()
	parser.add_argument("save_name", help="Name of model to save")
	parser.add_argument("--load_model", help="Name of model to load")
	parser.add_argument("--use_predictions", help="Use the network for predictions during search")
	args = parser.parse_args()

	if os.path.isdir(BIN_PATH + "\\data\\DataTuples"):
		shutil.rmtree(BIN_PATH + "\\data\\DataTuples")

	driver = Driver(args)
	driver.start()

if __name__ == "__main__":
	main()