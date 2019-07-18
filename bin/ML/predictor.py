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
                
                parse_start = time.clock()
                nn_input = self.parseString(data, highest_unit_count, have_y)
                parse_end = time.clock()
                
                #prediction_start = time.clock()
                output = self.network.predict(nn_input)  
                #prediction_end = time.clock()

                #fix_output_start = time.clock()
                #output = [np.ndarray.tolist(np.squeeze(output[0], axis=0)), np.ndarray.tolist(np.squeeze(output[1], axis=0))]
                #fix_output_end = time.clock()

           # print(output)
            #print("{} time to parse, {} time to predict".format(parse_end - parse_start, prediction_end - prediction_start))

            return output

def test():
    network = Network("2000_Policy_UnitWeights", "policy", False)

    line = "0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1.75,1.75,2,2,2.5,2.5,1.5,1.5,2.5,6,4,2,1,3.75,0,0.15,0,4.5,1.25,1.25,3,4.25,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,12,15,0,6720,12,0,0,0,0.0438,0.045,0.035"
    lines = ""
    for i in range(64):
        lines += line
        if i != 63:
            lines += "\n"
    #lines = "14,6,26,0,672,15,16,-1,0,0,18,6,0,0,0,25,9,-1,615,615,26,38,-1,672,672,0,0,0,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,291.5,0.01001,25,38,3982,2738,10,3,0,1.834e+04,0.0438,0.045,0.035\
    #\n14,6,26,0,672,15,16,-1,0,0,18,6,27,0,608,25,9,-1,615,615,26,38,-1,672,672,27,18,-1,608,608,0,0,0,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,191.5,0.01001,27,38,3982,2738,10,3,0,1.834e+04,0.0438,0.045,0.035\
    #\n14,6,26,0,672,15,16,-1,0,0,18,6,27,0,608,25,9,-1,615,615,26,38,-1,672,672,27,18,-1,608,608,28,5,-1,480,480,0,0,0,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,116.5,0.01001,27,38,3982,2738,10,3,0,1.834e+04,0.0438,0.045,0.035\
    #\n14,6,26,0,672,15,16,-1,0,0,18,6,27,0,608,25,9,-1,615,615,26,38,-1,672,672,27,18,-1,608,608,28,5,-1,480,480,29,4,-1,400,400,0,0,0,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16.48,0.01001,27,38,3982,2738,10,3,0,1.834e+04,0.0438,0.045,0.035\
    #\n14,6,26,0,64,15,16,-1,0,0,18,6,30,0,672,25,9,-1,7,7,26,38,-1,64,64,30,38,-1,672,672,0,0,0,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,166,74.37,29,46,4590,2130,7,6,0,2.047e+04,0.0438,0.045,0.035"
    
    lines = "14,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92.96,0,14,15,272,6448,13,0,0,0,0.0438,0.045,0.035\
\n13,4,-1,400,400,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.4576,0,12,15,96,6624,12,0,0,0,0.0438,0.045,0.035\
\n13,4,-1,400,400,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.4576,0,12,15,96,6624,12,0,0,0,0.0438,0.045,0.035\
\n13,4,-1,400,400,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.4576,0,12,15,96,6624,12,0,0,0,0.0438,0.045,0.035\
\n13,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
\n13,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
\n13,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
\n13,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
\n13,5,-1,480,480,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.2288,0,12,15,48,6672,12,0,0,0,0.0438,0.045,0.035\
\n13,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
\n13,33,-1,81,81,14,4,-1,400,400,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.3896,0,13,15,191,6529,12,0,0,0,0.0438,0.045,0.035\
\n13,4,-1,400,400,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.4576,0,12,15,96,6624,12,0,0,0,0.0438,0.045,0.035\
\n14,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92.96,0,14,15,272,6448,13,0,0,0,0.0438,0.045,0.035\
\n13,4,-1,210,210,14,4,-1,400,400,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.3216,0,12,15,286,6434,12,0,0,0,0.0438,0.045,0.035\
\n13,4,-1,305,305,14,33,-1,272,272,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.3896,0,13,15,191,6529,12,0,0,0,0.0438,0.045,0.035"

    #lines = "0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,12,15,0,6720,12,0,0,0,0.0438,0.045,0.035\
    #\n14,6,-1,774,774,15,33,-1,6,6,16,6,-1,1040,1040,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.507202,0,13,23,762,5958,12,0,0,0,0.0438,0.045,0.035\
    #\n14,6,-1,1040,1040,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,60.6976,0,12,23,496,6224,12,0,0,0,0.0438,0.045,0.035"

    print(network.predict(line, 0, False))

#test()
