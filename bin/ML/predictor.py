import os
import tensorflow as tf
import numpy as np
import model
import time
import json

BIN_PATH = "C:\\School Work\\BOSS\\bin"
PARSER_PATH = BIN_PATH + "\\ML"
MODELS_PATH = PARSER_PATH + "\\models"

class Network:
    def __init__(self, network_name, network_type, use_gpu=False):
        if not use_gpu:
            config = tf.ConfigProto(
            device_count = {'GPU': 0}
            )
            self.sess = tf.Session(config=config)
            print("NO GPU!!!")
        else:
            self.sess = tf.Session()
        tf.keras.backend.set_session(self.sess)

        NUM_PROTOSS_UNITS = 70
        self.num_unit_features = 6
        cpu_workers = 4
        self.extra_features = 12
        #self.feature_shape = (MAX_NUM_UNITS * NUM_UNIT_FEATURES) + EXTRA_FEATURES
        self.policy_shape = NUM_PROTOSS_UNITS
        self.value_shape = 0
        self.name = network_name
        self.learning_rate = 1e-4
        self.batch_size = 1
        self.loadNetwork(network_type)
        self.network.predict((np.zeros((1, 1, self.num_unit_features)), np.zeros((1, self.extra_features))))

        self.session = tf.keras.backend.get_session()
        self.graph = tf.get_default_graph()
        self.graph.finalize()

    def loadNetwork(self, network_type):      
        if network_type == "policy":
            self.network = model.RelationsPolicyNetwork(self.num_unit_features, self.extra_features, self.policy_shape, self.name, 
                                                self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        elif network_type == "value":
            self.network = model.IntegralValueNN(self.feature_shape, self.value_shape, self.name, 
                                                self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        elif network_type == "both":
            self.network = model.PolicyAndValueNetwork(self.feature_shape, self.policy_shape, self.value_shape,
                self.name, self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        else:
            print("invalid network type")
            assert False
        self.network.load(MODELS_PATH + "/" + self.name + ".h5")

    #def parseString(self, csv_string, shape):
    #    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
    #    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
    #    d = tf.squeeze(data)
    #    x = d[-(shape+1):]
    #    return x

    #def parseStringPolicy(self, csv_string):
    #    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
    #    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
    #    x = tf.squeeze(data)
    #    # adds zeroes for missing units
    #    x = tf.expand_dims(tf.concat([x, tf.zeros(self.feature_shape - tf.size(x))], 0), axis=0)
    #    #x.set_shape(self.feature_shape,)
    #    return x      

    def parseStringPolicy(self, csv_string, have_policy=False):
        split_string = csv_string.split(",")
        if have_policy:
            units = split_string[:-(self.extra_features + self.policy_shape)]
            units = np.expand_dims(np.reshape(units, (int(len(units)/self.num_unit_features), self.num_unit_features)), axis=0)
            extra_features = np.expand_dims(split_string[-(self.extra_features + self.policy_shape):-self.policy_shape], axis=0)
            policy = np.expand_dims(split_string[-self.policy_shape:], axis=0)
            return (units, extra_features), policy
        else:
            units = split_string[:-self.extra_features]
            units = np.expand_dims(np.reshape(units, (int(len(units)/self.num_unit_features), self.num_unit_features)), axis=0)
            extra_features = np.expand_dims(split_string[-self.extra_features:], axis=0)
            return (units, extra_features)
        
    def evaluate(self, data):
        nn_input, policy = self.parseStringPolicy(data, True)
        output = self.network.model.evaluate(nn_input, policy, steps=1)
        return output

    def compare(self, data):
        nn_input, policy = self.parseStringPolicy(data, True)
        predicted_policy = np.squeeze(self.network.predict(nn_input), axis=0)
        actual_policy = np.squeeze(policy, axis=0)
        for actual, predicted in zip(actual_policy, predicted_policy):
            print(actual + "\t\t", predicted)

    def predict(self, data):
        #parse_start = time.clock()
        nn_input = self.parseStringPolicy(data)
        #parse_end = time.clock()

        #prediction_start = time.clock
        output = np.ndarray.tolist(np.squeeze(self.network.predict(nn_input), axis=0))
        #prediction_end = time.clock()

        print(output)

        #print("{} time to parse, {} time to predict".format(parse_end - parse_start, prediction_end - prediction_start))
        return output

def test():
    network = Network("asd", "policy", True)

    line = "2,0,0,0,200,109.9,2,-1,0,0,200,109.9,32,0,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,3,-1,0,0,0,0,3,-1,0,0,0,0,5,37,0,240,0,0,4,-1,0,0,0,0,4,-1,0,0,0,0,15,-1,0,0,0,0,32,-1,0,0,0,0,19,-1,0,0,0,0,10,36,0,475,0,0,17,-1,0,0,0,0,5,38,0,672,0,0,28,-1,475,475,0,0,19,-1,240,240,0,0,19,-1,672,672,0,0,487.8,140.2,36,46,1712,2288,18,6,0,0.0438,0.045,0.035"
    network.predict(line)

    line = "2,31,0,5,200,59.35,2,-1,0,0,200,59.35,32,33,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,32,-1,0,0,0,0,3,-1,0,0,0,0,3,-1,0,0,0,0,5,32,0,405,0,0,4,-1,0,0,0,0,4,-1,0,0,0,0,15,-1,0,0,0,0,32,-1,5,5,0,0,19,-1,405,405,0,0,10,-1,960,960,0,0,98.81,0.09,26,46,267,3733,17,6,0,0.0438,0.045,0.035"
    network.predict(line)

#test()