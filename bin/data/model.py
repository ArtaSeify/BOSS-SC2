import tensorflow as tf
from tensorflow.keras import layers
import os
import math
import numpy as np
from datetime import datetime

# from data_loader import DataLoader

class CustomTensorBoard(tf.keras.callbacks.TensorBoard):
    def __init__(self, model, *args, **kwargs):
        super(CustomTensorBoard, self).__init__(*args, **kwargs)
        self.model = model
    
    def get_lr(self):
        return tf.keras.backend.eval(self.model.optimizer.lr)

    def on_epoch_end(self, epoch, logs=None):
  #     test_dataset = DataLoader(self.model.feature_shape, self.model.prediction_shape, True, self.model.batch_size, 8)
        # test_iterator = test_dataset.make_iterator(sess, [args.testset_file])
        # testset_samples = sum(1 for line in open(args.testset_file))

  #       evaluations.append(network.evaluate(test_iterator, floor(testset_samples/self.model.batch_size), verbose))

        logs.update({"learning rate": np.float_(self.get_lr())
                     })
        super(CustomTensorBoard, self).on_epoch_end(epoch, logs)

class Model():
    def __init__(self):
        return

class IntegralValueNN(Model):
    def __init__(self, input_shape, output_shape, model_name, batch_size, learning_rate, model_path, create_network=True):
        self.model_name = model_name
        self.model_path = model_path
        self.feature_shape = input_shape
        self.prediction_shape = output_shape
        self.batch_size = batch_size
        self.epochs = 0
        
        if create_network:
            self.create(input_shape, output_shape, model_name, batch_size, learning_rate)
        else:
            self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)

    def percent_error(self, y_true, y_pred):
        return tf.math.multiply(tf.math.divide(tf.math.abs(tf.math.subtract(y_true, y_pred)), tf.math.maximum(y_true, 1)), 100)

    def exponential_decay(self, epoch, lr):
        decay_rate = 0.70
        reduce_every_epochs = 1.0
        return lr * pow(decay_rate, math.floor((epoch+1) / reduce_every_epochs))

    def create(self, input_shape, output_shape, model_name, batch_size, learning_rate):
        inputs = tf.keras.Input(shape=(input_shape, ))
        layer = layers.Dense(2048, activation='elu')(inputs)
        layer = layers.Dense(2048, activation='elu')(layer)
        layer = layers.Dropout(0.30)(layer)
        layer = layers.Dense(1024, activation='elu')(layer)
        layer = layers.Dropout(0.30)(layer)
        layer = layers.Dense(1024, activation='elu')(layer)
        layer = layers.Dropout(0.30)(layer)
        layer = layers.Dense(512, activation='elu')(layer)
        prediction = layers.Dense(output_shape)(layer)

        self.model = tf.keras.Model(inputs=inputs, outputs=prediction)

        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path, monitor='loss', save_best_only=True, mode='min')

        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate),
                loss='mse',
                metrics=['mae', self.percent_error])

    def train(self, iterator, epochs, steps_per_epoch, verbose):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, callbacks=
                                                                    [CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name))
                                                                    , write_graph=False, batch_size=self.batch_size), 
                                                                    self.checkpoint])

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        return self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose)

    def predict_on_batch(self, nn_input):
        return self.model.predict_on_batch(nn_input)

    def save(self, path):
        tf.keras.models.save_model(self.model, path)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, custom_objects={"percent_error": self.percent_error})