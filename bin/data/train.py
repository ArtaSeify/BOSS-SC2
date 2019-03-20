import tensorflow as tf
from tensorflow.keras import layers
from data_loader import DataLoader
import model
from math import floor
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("save_name", help="Name of model to save")
parser.add_argument("trainset_file", help="Path to datafile to use for training")
parser.add_argument("testset_file", help="Path to datafile to use for testing")
parser.add_argument("--load_model", help="Name of model to load")
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
# NUM_PROTOSS_UNITS = 68
MAX_NUM_UNITS = 35
cpu_workers = 12
feature_shape = (MAX_NUM_UNITS * 15) + 13
prediction_shape = 1
learning_rate = 1e-4
epochs = 30
verbose = 1
batch_size = 32
shuffle = True

trainset_samples = sum(1 for line in open(args.trainset_file))
testset_samples = sum(1 for line in open(args.testset_file))
# trainset_samples = 10000
# testset_samples = 1000

train_dataset = DataLoader(feature_shape, prediction_shape, trainset_samples, testset_samples, shuffle, batch_size, cpu_workers)
train_iterator = train_dataset.make_iterator(sess, [args.trainset_file])

test_dataset = DataLoader(feature_shape, prediction_shape, trainset_samples, testset_samples, shuffle, batch_size, cpu_workers)
test_iterator = test_dataset.make_iterator(sess, [args.testset_file])


network = model.IntegralValueNN(feature_shape, prediction_shape, args.save_name, batch_size, learning_rate, "C:\\School Work\\BOSS\\bin\\data\\models\\" + args.save_name + ".h5", True if args.load_model is None else False)
if args.load_model:
	network.load("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.load_model + ".h5")

evaluations = []
# train and evaluate
network.train(train_iterator, epochs, floor(trainset_samples/batch_size), verbose)	
evaluations.append(network.evaluate(test_iterator, floor(testset_samples/batch_size), verbose))
# network.train(train_iterator, epochs, None, verbose)	
# evaluations.append(network.evaluate(test_iterator, None, verbose))

# network.train(train_iterator, 1, 100, verbose)
	# network.epochs += 1
	# evaluations.append(network.evaluate(test_iterator, 100, verbose))

print(evaluations)
if not os.path.isdir(os.path.join(os.getcwd(), "logs\\evaluations\\")):
	os.makedirs(os.path.join(os.getcwd(), "logs\\evaluations\\"))
with open(os.path.join(os.getcwd(), "logs\\evaluations\\" + args.save_name + ".txt"), 'w') as outputFile:
	for val in evaluations:
		outputFile.write(str(val))
		outputFile.write("\n")

# save data
#network.save("C:\\School Work\\BOSS\\bin\\data\\models\\" + args.save_name + ".h5")