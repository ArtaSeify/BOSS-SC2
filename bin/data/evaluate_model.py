import tensorflow as tf
from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor
from random import shuffle
import numpy as np
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--load_model", help="Name of model to load")

args = parser.parse_args()

# creating iterators to go through the datasets using the dataset API
test_file = open('test.csv', 'r')
samples = test_file.readlines()

prediction_shape = 1
feature_shape = len(samples[0].split(",")) - prediction_shape
shuffle(samples)

def parse(csv_string):
	    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
	    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
	    d = tf.squeeze(data)
	    x, y = d[-(feature_shape+1):-prediction_shape], d[-prediction_shape:]
	    x.set_shape(feature_shape,)
	    y.set_shape(prediction_shape,)
	    return x, y

sess = tf.Session()
tf.keras.backend.set_session(sess)

# network
learning_rate = 1e-3
verbose = 1

#network = model.IntegralValueNN(feature_shape, prediction_shape, learning_rate)
network = tf.keras.models.load_model(os.path.join(os.getcwd(), os.path.join("models", args.load_model + ".h5")))

for sample in samples:
	data_x, pred_y = parse(sample)

	data_x = tf.expand_dims(data_x, axis=0)
	# train and evaluate
	print(network.predict(data_x, steps=1, verbose=verbose))
	print(sample.split(",")[feature_shape])