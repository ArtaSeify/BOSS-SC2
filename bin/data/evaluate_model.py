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
parser.add_argument("load_model", help="Name of model to load")
parser.add_argument("testset_file", help="Path to the test set file")
args = parser.parse_args()

# sess = tf.Session()
# tf.keras.backend.set_session(sess)
use_gpu = True
if not use_gpu:
    config = tf.ConfigProto(
    device_count = {'GPU': 0}
    )
    sess = tf.Session(config=config)
else:
    sess = tf.Session()
tf.keras.backend.set_session(sess)

# constants
NUM_PROTOSS_UNITS = 68
MAX_NUM_UNITS = 35
cpu_workers = 2
feature_shape = (MAX_NUM_UNITS * 15) + 12
prediction_shape = 1
verbose = 1
batch_size = 32
shuffle = True

testset_samples = sum(1 for line in open(args.testset_file))
# testset_samples = 100
test_dataset = DataLoader(feature_shape, prediction_shape, 0, testset_samples, shuffle, batch_size, cpu_workers)
test_iterator = test_dataset.make_iterator(sess, [args.testset_file])

network = model.IntegralValueNN(feature_shape, prediction_shape, args.load_model, batch_size, 1e-4, "C:\\School Work\\BOSS\\bin\\data\\models\\" + args.load_model + ".h5", False)
network.load("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.load_model + ".h5")

# test the network
print(network.evaluate(test_iterator, floor(testset_samples/batch_size), verbose))
