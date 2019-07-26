import os
import tensorflow as tf
import numpy as np
import model
import time
import json

if os.name == "nt":
    WINDOWS = True
else:
    WINDOWS = False

if WINDOWS:
    BIN_PATH = "C:\\School Work\\BOSS\\bin"
    MODELS_PATH = BIN_PATH + "\\ML\\models"
    DATA_PATH = BIN_PATH + "\\SavedStates"
else:
    BIN_PATH = "/home/seify/NEWBOSS/bin"
    MODELS_PATH = BIN_PATH + "/ML/models"
    DATA_PATH = BIN_PATH + "/SavedStates"

class Network:
    def __init__(self, network_name, network_type, use_gpu=False):
        if not use_gpu:
            config = tf.ConfigProto(
            device_count = {'GPU': 0}
            )
            self.sess = tf.Session(config=config)
        else:
            self.sess = tf.Session()
        tf.keras.backend.set_session(self.sess)
        NUM_PROTOSS_UNITS = 70
        self.num_unit_features = 5
        cpu_workers = 6
        self.extra_features = 13 + (2 * NUM_PROTOSS_UNITS)
        self.policy_shape = NUM_PROTOSS_UNITS
        self.name = network_name
        self.learning_rate = 1e-4
        self.batch_size = 1
        self.loadNetwork(network_type)
        self.network.predict((np.zeros((1, 1, self.num_unit_features)), np.zeros((1, self.extra_features))))

        tf.get_default_graph().finalize()

    def loadNetwork(self, network_type):      
        if network_type == "policy":
            self.value_shape = 0
            self.network = model.RelationsPolicyNetwork(self.num_unit_features, self.extra_features, self.policy_shape, self.name, 
                                                self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        elif network_type == "value":
            self.value_shape = 1
            self.network = model.IntegralValueNN(self.feature_shape, self.value_shape, self.name, 0,
                                                self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        elif network_type == "both":
            self.value_shape = 1
            self.network = model.RelationsPolicyAndValueNetwork(self.num_unit_features, self.extra_features, self.policy_shape, 0, self.name, 
                                                self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        else:
            print("invalid network type")
            assert False
        self.network.load(MODELS_PATH + "/" + self.name + ".h5")

    def parseStringWithY(self, csv_strings):
        batch_string = csv_strings.split("\n")
        split_strings = np.array([x.split(",") for x in batch_string])

        # policy only
        if self.value_shape == 0:
            units = split_string[:-(self.extra_features + self.policy_shape)]
            units = np.expand_dims(np.reshape(units, (int(len(units)/self.num_unit_features), self.num_unit_features)), axis=0)
            extra_features = np.expand_dims(split_string[-(self.extra_features + self.policy_shape):-self.policy_shape], axis=0)
            policy = np.expand_dims(split_string[-self.policy_shape:], axis=0)
            return (units, extra_features), policy

        # policy and value
        else:
            units = split_strings[:-(self.extra_features + self.policy_shape + self.value_shape)]
            units = np.expand_dims(np.reshape(units, (int(len(units)/self.num_unit_features), self.num_unit_features)), axis=0)
            extra_features = np.expand_dims(split_strings[-(self.extra_features + self.policy_shape + self.value_shape):-(self.policy_shape + self.value_shape)], axis=0)
            policy = np.expand_dims(split_strings[-self.policy_shape:-self.value_shape], axis=0)
            value = np.expand_dims(split_strings[-self.value_shape:], axis=0)

            return (units, extra_features), (policy, value)


    def parseString(self, csv_strings, highest_unit_count, have_y=False):
        if highest_unit_count == 0:
            highest_unit_count = 1

        if have_y:
            return self.parseStringWithY(csv_strings)
        else:
            return self.parseStringWithoutY(csv_strings, highest_unit_count)
        
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

    def parseStringWithoutY(self, csv_strings, highest_unit_count):
        batch_string = csv_strings.split("\n")
        split_strings = np.array([x.split(",") for x in batch_string])
        batch_size = len(split_strings)

        all_units = []
        all_extra_features = [] 

        for x in split_strings:
            units = x[:-self.extra_features]
            all_units.append(np.pad(units, (0, (5 * highest_unit_count) - len(units)), mode='constant', constant_values=0))
            all_extra_features.append(x[-self.extra_features:])

        corrected_units = np.reshape(np.array(all_units), (batch_size, highest_unit_count, self.num_unit_features))
        corrected_extra_features = np.reshape(np.array(all_extra_features), 
                                                (batch_size, self.extra_features))

        return (corrected_units, corrected_extra_features)

    def predict(self, data, highest_unit_count, have_y=False):
        #print(data)
        #print(highest_unit_count)
        with self.sess.as_default():
            if self.value_shape == 0:
                #parse_start = time.clock()
                nn_input = self.parseString(data, highest_unit_count, have_y)
                #parse_end = time.clock()

                #prediction_start = time.clock()
                output = self.network.predict(nn_input)
                #prediction_end = time.clock()

                #fix_output_start = time.clock()
                #output = np.ndarray.tolist(np.squeeze(output, axis=0))
                #fix_output_end = time.clock()

            else:
                
                #parse_start = time.clock()
                nn_input = self.parseString(data, highest_unit_count, have_y)
                #parse_end = time.clock()
                
                #prediction_start = time.clock()
                output = self.network.predict(nn_input)  
                #prediction_end = time.clock()

                #fix_output_start = time.clock()
                #output = [np.ndarray.tolist(np.squeeze(output[0], axis=0)), np.ndarray.tolist(np.squeeze(output[1], axis=0))]
                #fix_output_end = time.clock()

            #print(output)
            #print("{} time to parse, {} time to predict".format(parse_end - parse_start, prediction_end - prediction_start))

            return output

def test():
    network = Network("1000_PolicyAndValue_UnitWeights", "both", True)

    line = "0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1.75,1.75,2,2,2.5,2.5,1.5,1.5,2.5,6,4,2,1,3.75,0,0.15,0,4.5,1.25,1.25,3,4.25,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,12,15,0,6720,12,0,0,0,0.0438,0.045,0.035"
    lines = ""
    for i in range(64):
        lines += line
        if i != 63:
            lines += "\n"
    for i in range(10):
        network.predict(lines, 0, False)

#test()
