import tensorflow as tf
from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("save_name", help="Name of model to save")
parser.add_argument("data_file", help="Path to datafile to use for training")
parser.add_argument("--load_model", help="Name of model to load")
args = parser.parse_args()

sess = tf.Session()
tf.keras.backend.set_session(sess)

# constants
cpu_workers = 4
feature_shape = 332
prediction_shape = 1
learning_rate = 1e-4
epochs = 50
verbose = 1
batch_size = 16

dataset = DataLoader(feature_shape, prediction_shape, True, batch_size, cpu_workers)
train_iterator = dataset.make_iterator(sess, [args.data_file])
trainset_samples = sum(1 for line in open(args.data_file))

network = model.IntegralValueNN(feature_shape, prediction_shape, args.save_name, batch_size, learning_rate, "C:\\School Work\\BOSS\\bin\\data\\models\\" + args.save_name + ".h5", True if args.load_model is None else False)
if args.load_model:
	network.load("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.load_model + ".h5")

# train and evaluate
network.train(train_iterator, epochs, floor(trainset_samples/batch_size), verbose)

# save data
#network.save("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.save_name + ".h5")