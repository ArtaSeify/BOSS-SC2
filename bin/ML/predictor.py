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
        else:
            self.sess = tf.Session()
        tf.keras.backend.set_session(self.sess)

        NUM_PROTOSS_UNITS = 70
        NUM_UNIT_FEATURES = 7
        MAX_NUM_UNITS = 100
        EXTRA_FEATURES = 9
        cpu_workers = 4
        self.feature_shape = (MAX_NUM_UNITS * NUM_UNIT_FEATURES) + EXTRA_FEATURES
        self.policy_shape = NUM_PROTOSS_UNITS
        self.value_shape = 0
        self.name = network_name
        self.learning_rate = 1e-4
        self.batch_size = 1
        self.loadNetwork(network_type)

    def loadNetwork(self, network_type):      
        if network_type == "policy":
            self.network = model.PolicyNetwork(self.feature_shape, self.policy_shape, self.name, 
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

    def parseStringPolicy(self, csv_string):
        split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
        data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
        x = tf.squeeze(data)
        # adds zeroes for missing units
        x = tf.expand_dims(tf.concat([x, tf.zeros(self.feature_shape - tf.size(x))], 0), axis=0)
        #x.set_shape(self.feature_shape,)
        return x      


    def evaluate(self, x_val, y_pred):
        nn_input = self.parseStringPolicy(x_val)
        output = self.network.model.evaluate(nn_input, actual, steps=1)
        return output

    #def predictParsed(self, data):
    #    nn_input = tf.convert_to_tensor(list(map(lambda x: self.parseString(x, self.feature_shape), data.split("\n"))))
    #    # output = np.ndarray.tolist(np.squeeze(self.network.predict_on_batch(nn_input)))
    #    output = self.network.predict_on_batch(nn_input)
    #    # if isinstance(output, float):
    #        # output = [output]
    #    return output

    def predict(self, data):
        #parse_start = time.clock()
        nn_input = self.parseStringPolicy(data)
        #parse_end = time.clock()
        
        #prediction_start = time.clock()
        output = list(map(float, self.network.predict(nn_input)[0]))
        #prediction_end = time.clock()

        #print("{} time to parse, {} time to predict".format(parse_end - parse_start, prediction_end - prediction_start))
        return output

#def test():
#    line = "0,2,-1,0,0,200,50,1,2,-1,0,0,200,50,2,32,-1,0,0,0,0,3,32,-1,0,0,0,0,4,32,-1,0,0,0,0,5,32,-1,0,0,0,0,6,32,-1,0,0,0,0,7,32,-1,0,0,0,0,8,32,-1,0,0,0,0,9,32,-1,0,0,0,0,10,32,-1,0,0,0,0,11,32,-1,0,0,0,0,12,32,-1,0,0,0,0,13,32,-1,0,0,0,0,14,32,-1,0,0,0,0,15,32,-1,0,0,0,0,16,32,-1,0,0,0,0,17,32,-1,0,0,0,0,18,32,-1,0,0,0,0,19,32,-1,0,0,0,0,20,32,-1,0,0,0,0,21,32,-1,0,0,0,0,22,32,-1,0,0,0,0,23,32,-1,0,0,0,0,24,32,-1,0,0,0,0,25,3,-1,0,0,0,0,26,3,-1,0,0,0,0,27,5,-1,0,0,0,0,28,4,-1,0,0,0,0,29,4,-1,0,0,0,0,30,15,-1,0,0,0,0,225,128,23,46,0,6720,17,6,0"
#    network = Network("test", "policy")

#    print(network.predict(line))

#test()