import tensorflow as tf
from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor, ceil
import os
import argparse
import json

parser = argparse.ArgumentParser()
parser.add_argument("save_name", help="Name of model to save")
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
EXTRA_FEATURES = 12 + NUM_PROTOSS_UNITS
cpu_workers = 6
policy_shape = NUM_PROTOSS_UNITS
value_shape = 1
learning_rate = 1e-4
epochs = 20 if not args.epochs else int(args.epochs)
verbose = 1
batch_size = 32
shuffle = True
twoHeads = False

if not os.path.isdir("models"):
    os.makedirs("models")

network = model.RelationsPolicyAndValueNetwork(NUM_UNIT_FEATURES, EXTRA_FEATURES, policy_shape, args.save_name, batch_size, learning_rate, "models/" + args.save_name + ".h5", True)
#network = model.RelationsPolicyNetwork(NUM_UNIT_FEATURES, EXTRA_FEATURES, policy_shape, args.save_name, batch_size, learning_rate, "models/" + args.save_name + ".h5", True if args.load_model is None else False)
# network = model.PolicyAndValueNetwork(feature_shape, policy_shape, value_shape, args.save_name, batch_size, learning_rate, "models/" + args.save_name + ".h5", True if args.load_model is None else False)
if args.load_model:
    #network.load("models/" + args.load_model + ".h5")
    #network.compile()
    network.model.load_weights("models/" + args.load_model + ".h5", by_name=True)
network.save("models/" + args.save_name + ".h5")
# evaluations = []
# # train and evaluate
# train_step = max(int(trainset_samples / batch_size), 1)
# valid_step = max(int(validset_samples / batch_size), 1)
            
# network.train(train_iterator, epochs, train_step, verbose, valid_iterator, valid_step)	

#if args.testset:
#    evaluations.append(network.evaluate(test_iterator, int(floor(testset_samples/batch_size)), verbose))
#    print(evaluations)

#    if not os.path.isdir(os.path.join(os.getcwd(), "logs/evaluations/")):
#        os.makedirs(os.path.join(os.getcwd(), "logs/evaluations/"))

#    with open(os.path.join(os.getcwd(), "logs/evaluations/" + args.save_name + ".txt"), 'w') as outputFile:
#        for val in evaluations:
#            outputFile.write(str(val))
#            outputFile.write("\n")

# save data
#network.save("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.save_name + ".h5")