import tensorflow as tf
from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor
import os
import argparse
import json

parser = argparse.ArgumentParser()
parser.add_argument("save_name", help="Name of model to save")
parser.add_argument("trainset", help="Path to datafile to use for training")
parser.add_argument("--testset", help="Path to datafile to use for testing")
parser.add_argument("--load_model", help="Name of model to load")
parser.add_argument("--epochs", help="Number of epochs to train")
args = parser.parse_args()

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
NUM_PROTOSS_UNITS = 70
NUM_UNIT_FEATURES = 6
#MAX_NUM_UNITS = 100
EXTRA_FEATURES = 12
cpu_workers = 4
#feature_shape = (MAX_NUM_UNITS * NUM_UNIT_FEATURES) + EXTRA_FEATURES
policy_shape = NUM_PROTOSS_UNITS
value_shape = 0
learning_rate = 1e-4
epochs = 15 if not args.epochs else int(args.epochs)
verbose = 1
batch_size = 16
shuffle = True
twoHeads = False

trainset_samples = sum(1 for line in open(args.trainset))
if args.testset:
    testset_samples = sum(1 for line in open(args.testset))
# trainset_samples = 10000
# testset_samples = 1000

train_dataset = DataLoader(NUM_UNIT_FEATURES, EXTRA_FEATURES, policy_shape, value_shape, trainset_samples, testset_samples if args.testset else 0, twoHeads, shuffle, batch_size, cpu_workers)
train_iterator = train_dataset.make_iterator(sess, [args.trainset])

if args.testset:
    test_dataset = DataLoader(NUM_UNIT_FEATURES, EXTRA_FEATURES, policy_shape, value_shape, trainset_samples, testset_samples, twoHeads, shuffle, batch_size, cpu_workers)
    test_iterator = test_dataset.make_iterator(sess, [args.testset])

if not os.path.isdir("models"):
    os.makedirs("models")

network = model.RelationsPolicyNetwork(NUM_UNIT_FEATURES, EXTRA_FEATURES, policy_shape, args.save_name, batch_size, learning_rate, "models/" + args.save_name + ".h5", True if args.load_model is None else False)
# network = model.PolicyAndValueNetwork(feature_shape, policy_shape, value_shape, args.save_name, batch_size, learning_rate, "models/" + args.save_name + ".h5", True if args.load_model is None else False)
if args.load_model:
    network.load("models/" + args.load_model + ".h5")

import numpy as np
#network.model.fit(np.random.randint(20, size=(32, 30, 7)), np.random.randint(10, size=(32, 70)), batch_size=32)
evaluations = []
# train and evaluate
network.train(train_iterator, epochs, int(floor(trainset_samples/batch_size)), verbose)	

if args.testset:
    evaluations.append(network.evaluate(test_iterator, int(floor(testset_samples/batch_size)), verbose))
    print(evaluations)

    if not os.path.isdir(os.path.join(os.getcwd(), "logs/evaluations/")):
        os.makedirs(os.path.join(os.getcwd(), "logs/evaluations/"))

    with open(os.path.join(os.getcwd(), "logs/evaluations/" + args.save_name + ".txt"), 'w') as outputFile:
        for val in evaluations:
            outputFile.write(str(val))
            outputFile.write("\n")

# save data
#network.save("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.save_name + ".h5")