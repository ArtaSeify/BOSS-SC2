import os
import tensorflow as tf
import numpy as np
import model
from data_filter_functions import parseLine, createUnitDict

BIN_PATH = "C:\\School Work\\BOSS\\bin"
PARSER_PATH = BIN_PATH + "\\data"
MODELS_PATH = PARSER_PATH + "\\models"
DATA_PATH = BIN_PATH + "\\data\\DataTuples\\PredictionData"
DATA_FILE = os.path.join(DATA_PATH, "CurrentStateData.csv")

class Network:
	def __init__(self, network_name):
		self.sess = tf.Session()
		tf.keras.backend.set_session(self.sess)

		self.name = network_name + ".h5"
		self.feature_shape = 332
		self.prediction_shape = 1
		self.learning_rate = 1e-4
		self.epochs = 1
		self.verbose = 1
		self.batch_size = 4

		self.mins_per_worker_per_sec = "0.045"
		self.gas_per_worker_per_sec = "0.07"

		self.loadNetwork()
		self.unit_dict = createUnitDict("C:\\School Work\\BOSS\\bin\\data\\ActionData.txt")

	def loadNetwork(self):		
		self.network = model.IntegralValueNN(self.feature_shape, self.prediction_shape, self.name, 
											self.batch_size, self.learning_rate, False)
		self.network.load(MODELS_PATH + "\\" + self.name)

	def parseString(self, csv_string):
	    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
	    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
	    d = tf.squeeze(data)
	    x = d[-(self.feature_shape+1):]
	    return x

	def predict(self, data):
		nn_input = tf.convert_to_tensor(list(map(lambda x: self.parseString(parseLine(x, self.unit_dict,
				self.mins_per_worker_per_sec, self.gas_per_worker_per_sec, 1)), data.split("\n"))))
		
		output = np.ndarray.tolist(np.squeeze(self.network.predict_on_batch(nn_input)))
		if isinstance(output, float):
			output = [output]

		return output

# line = "[[0,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[1,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[2,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[3,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[4,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[5,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[6,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[7,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[8,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[9,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[10,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[11,0,0,-1,29,0,0,0,0,0,0,0,0,0,0],[12,0,0,-1,4,0,29,13,0,0,272,0,0,200,50],[13,0,-1,12,29,0,0,0,0,272,272,0,0,0,0]],[13],[],[],0,0,0,13,15,0,0,12,0,0,0,1,29,[0,0,0,0,0]"
# unit_dict = createUnitDict("C:\\School Work\\BOSS\\bin\\data\\ActionData.txt")
# mins_per_worker_per_sec = "0.045"
# gas_per_worker_per_sec = "0.07"
# network = Network("testing", "10000")
# network.predict(line)
#predict(line + "\n" + line)