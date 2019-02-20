import tensorflow as tf
from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor
import os

data_file_folder = "state_data/Integral_2200"

with open(data_file_folder + "/train.csv", 'r') as temp:
	feature_shape = len(temp.readline().split(",")) - 1

parser = argparse.ArgumentParser()
parser.add_argument("save_name", help="Name of model to save")
parser.add_argument("--load_model", help="Name of model to load")

args = parser.parse_args()

sess = tf.Session()
tf.keras.backend.set_session(sess)

# creating iterators to go through the datasets using the dataset API
batch_size = 32
cpu_workers = 4
prediction_shape = 1

dataset = DataLoader(feature_shape, prediction_shape, batch_size, cpu_workers)

train_iterator = dataset.make_iterator(sess, [data_file_folder + "/train.csv"])
trainset_samples = sum(1 for line in open(data_file_folder + "/train.csv"))

test_iterator = dataset.make_iterator(sess, [data_file_folder + "/test.csv"])
testset_samples = sum(1 for line in open(data_file_folder + "/test.csv"))

# network
learning_rate = 1e-4

epochs = 10
verbose = 1

network = model.IntegralValueNN(feature_shape, prediction_shape, args.save_name, batch_size, learning_rate)

# train and evaluate
network.train(train_iterator, epochs, floor(trainset_samples/batch_size), verbose)
network_result = network.evaluate(test_iterator, floor(testset_samples/batch_size), verbose)
print(network_result)

# save data
network.save(args.save_name + ".h5")
with open(os.path.join(os.path.join(os.getcwd(), "model_accuracy"), args.save_name + ".txt"), 'w') as writefile:
	for result in network_result:
		writefile.write(result)