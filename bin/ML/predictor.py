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
        self.extra_features = 13 + NUM_PROTOSS_UNITS
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
            self.network = model.IntegralValueNN(self.feature_shape, self.value_shape, self.name, 
                                                self.batch_size, self.learning_rate, MODELS_PATH + "\\" + self.name, False)
        elif network_type == "both":
            self.value_shape = 1
            self.network = model.RelationsPolicyAndValueNetwork(self.num_unit_features, self.extra_features, self.policy_shape, self.name, 
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

    def parseStringWithoutY(self, csv_strings):
        batch_string = csv_strings.split("\n")
        split_strings = np.array([x.split(",") for x in batch_string])

        # batch size 1
        if len(split_strings) == 1:
            units = split_strings[0][:-self.extra_features]
            units = np.reshape(units, (1, int(len(units)/self.num_unit_features), self.num_unit_features))
            extra_features = np.expand_dims(split_strings[0][-self.extra_features:], axis=0)
            return (units, extra_features)
        
        else:
            all_units = []
            all_extra_features = np.array([]).reshape(0,self.extra_features)
            highest_unit_count = 0
            for x in split_strings:
                units = x[:-self.extra_features]
                num_units = int(len(units)/self.num_unit_features)
                highest_unit_count = max(highest_unit_count, num_units)
                units = np.reshape(units, (num_units, self.num_unit_features))

                all_units.append(units)
                extra_features = np.expand_dims(x[-self.extra_features:], axis=0)
                all_extra_features = np.concatenate([all_extra_features,extra_features], axis=0)

            corrected_units = np.array([]).reshape(0,highest_unit_count,self.num_unit_features)
            for unit_list in all_units:
                pad = np.zeros((highest_unit_count - unit_list.shape[0],self.num_unit_features))
                corrected_units = np.concatenate([corrected_units, 
                                                    np.expand_dims(np.concatenate([unit_list, pad], axis=0), axis=0)],
                                                axis=0)

            return (corrected_units, all_extra_features)

    def parseString(self, csv_strings, have_y=False):
        if have_y:
            return self.parseStringWithY(csv_strings)
        else:
            return self.parseStringWithoutY(csv_strings)
        
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

    def predict(self, data, have_y=False):
        with self.sess.as_default():
            if self.value_shape == 0:
                #parse_start = time.clock()
                nn_input = self.parseString(data, have_y)
                #parse_end = time.clock()

                #prediction_start = time.clock()
                output = self.network.predict(nn_input)
                #prediction_end = time.clock()

                #fix_output_start = time.clock()
                #output = np.ndarray.tolist(np.squeeze(output, axis=0))
                #fix_output_end = time.clock()

            else:
                
                #parse_start = time.clock()
                nn_input = self.parseString(data, have_y)
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
    network = Network("testpolicy", "policy", False)

    lines = "12,3,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,12,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
             \n12,3,13,0,272,13,33,-1,272,272,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,15,0,6720,12,0,0,0,0.0438,0.045,0.035"

    # ,0,0,0,0.49,0.22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.29,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.017
    line = "12,3,-1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,12,15,0,6720,12,0,0,0,0.0438,0.045,0.035"
    #print(network.predict(line, False))
    #print(len(lines))
    network.predict(lines, False)

#test()