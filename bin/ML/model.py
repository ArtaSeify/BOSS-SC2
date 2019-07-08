import tensorflow as tf
from tensorflow.keras import layers
import os
import math
import numpy as np
from scipy.special import softmax
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

        self.checkpoint_best = tf.keras.callbacks.ModelCheckpoint(self.model_path.split(".")[0] + "_best.h5", monitor='loss', save_best_only=True, mode='min')
        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)
        
        if create_network:
            self.create(input_shape, output_shape, model_name, batch_size, learning_rate)

    def percent_error(self, y_true, y_pred):
        return tf.math.multiply(tf.math.divide(tf.math.abs(tf.math.subtract(y_true, y_pred)), tf.math.maximum(y_true, 1)), 100)

    def exponential_decay(self, epoch, lr):
        decay_rate = 0.70
        reduce_every_epochs = 1.0
        return lr * pow(decay_rate, math.floor((epoch+1) / reduce_every_epochs))

    def create(self, input_shape, output_shape, model_name, batch_size, learning_rate):
        inputs = tf.keras.Input(shape=(input_shape, ), name="state")
        layer = layers.Dense(2048, activation='elu')(inputs)
        layer = layers.Dense(1024, activation='elu')(layer)
        layer = layers.Dense(1024, activation='elu')(layer)
        #layer = layers.Dropout(0.30)(layer)
        #layer = layers.Dropout(0.30)(layer)
        layer = layers.Dense(512, activation='elu')(layer)
        layer = layers.Dense(512, activation='elu')(layer)
        prediction = layers.Dense(output_shape, name="value")(layer)

        self.model = tf.keras.Model(inputs=inputs, outputs=prediction)

        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate),
                loss='mse',
                metrics=['mae', self.percent_error])

    def train(self, iterator, epochs, steps_per_epoch, verbose, class_weight=None):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, 
                                callbacks=[CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name)), write_graph=False, batch_size=self.batch_size), 
                                        self.checkpoint, self.checkpoint_best],
                                class_weight = class_weight)

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        return self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose)

    def predict_on_batch(self, nn_input):
        output = self.model.predict_on_batch(nn_input)
        output = np.ndarray.tolist(np.squeeze(output))
        if isinstance(output, float):
           output = [output]
        return output

    def save(self, path):
        tf.keras.models.save_model(self.model, path)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, custom_objects={"percent_error": self.percent_error})

class PolicyNetwork(Model):
    def __init__(self, input_shape, output_shape, model_name, batch_size, learning_rate, model_path, create_network=True):
        self.model_name = model_name
        self.model_path = model_path
        self.feature_shape = input_shape
        self.prediction_shape = output_shape
        self.batch_size = batch_size
        self.epochs = 0

        self.checkpoint_best = tf.keras.callbacks.ModelCheckpoint(self.model_path.split(".")[0] + "_best.h5", monitor='categorical_accuracy', save_best_only=True, mode='max')
        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)
        
        if create_network:
            self.create(input_shape, output_shape, model_name, batch_size, learning_rate)

    def exponential_decay(self, epoch, lr):
        decay_rate = 0.70
        reduce_every_epochs = 1.0
        return lr * pow(decay_rate, math.floor((epoch+1) / reduce_every_epochs))

    def top_2_accuracy(self, y_true, y_pred):
        return tf.keras.metrics.top_k_categorical_accuracy(y_true, y_pred, k=2)

    def CCELogits(self, y_true, y_pred):
        return tf.keras.backend.categorical_crossentropy(y_true, y_pred, from_logits=True)

    def accuracy(self, y_true, y_pred):
        indices = tf.concat([tf.convert_to_tensor([[i] for i in range(self.batch_size)], dtype=tf.int64),
                             tf.expand_dims(tf.keras.backend.argmax(y_pred, axis=-1), 1)], 1)
        nonzeros = tf.math.divide(tf.math.count_nonzero(tf.gather_nd(y_true, indices)),self.batch_size)
        return nonzeros 

    # BEST MODEL SO FAR:
    # inputs = tf.keras.Input(shape=(input_shape, ))
    # layer = layers.Dense(512, activation='elu')(inputs)
    # layer = layers.Dense(256, activation='elu')(layer)
    # layer = layers.Dense(256, activation='elu')(layer)
    # layer = layers.Dense(128, activation='elu')(layer)
    # layer = layers.Dense(128, activation='elu')(layer)
    # prediction = layers.Dense(output_shape, activation='linear')(layer)
    def create(self, input_shape, output_shape, model_name, batch_size, learning_rate):
        inputs = tf.keras.Input(shape=(input_shape, ), name="state")
        layer = layers.Dense(512, activation='elu')(inputs)
        layer = layers.Dense(256, activation='elu')(layer)
        layer = layers.Dense(256, activation='elu')(layer)
        layer = layers.Dense(128, activation='elu')(layer)
        layer = layers.Dense(128, activation='elu')(layer)
        prediction = layers.Dense(output_shape, activation='linear', name="policy")(layer)
        #prediction = layers.Dense(output_shape, activation='softmax')(layer)

        self.model = tf.keras.Model(inputs=inputs, outputs=prediction)

        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate),
                #loss='categorical_crossentropy',
                #loss='kld',
                loss = self.CCELogits,
                metrics=['categorical_accuracy', self.top_2_accuracy, self.accuracy])

    def train(self, iterator, epochs, steps_per_epoch, verbose, class_weight=None):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, 
                                callbacks=[CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name)), write_graph=False, batch_size=self.batch_size), 
                                        self.checkpoint, self.checkpoint_best],
                                class_weight = class_weight)

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        return softmax(self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose))

    def predict_on_batch(self, nn_input):
        return np.ndarray.tolist(np.squeeze(softmax(self.model.predict_on_batch(nn_input))))

    def save(self, path):
        tf.keras.models.save_model(self.model, path)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, 
            custom_objects={"top_2_accuracy": self.top_2_accuracy, "CCELogits": self.CCELogits, "accuracy": self.accuracy})


class PolicyAndValueNetwork(Model):
    def __init__(self, input_shape, policy_shape, value_shape, model_name, batch_size, learning_rate, model_path, create_network=True):
        self.model_name = model_name
        self.model_path = model_path
        self.feature_shape = input_shape
        self.policy_shape = policy_shape
        self.value_shape = value_shape
        self.batch_size = batch_size
        self.learning_rate = learning_rate
        self.epochs = 0

        self.checkpoint_best = tf.keras.callbacks.ModelCheckpoint(self.model_path.split(".")[0] + "_best.h5", monitor='categorical_accuracy', save_best_only=True, mode='max')
        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)
        
        if create_network:
            self.create()

    def exponential_decay(self, epoch, lr):
        decay_rate = 0.70
        reduce_every_epochs = 1.0
        return lr * pow(decay_rate, math.floor((epoch+1) / reduce_every_epochs))

    def top_2_accuracy(self, y_true, y_pred):
        return tf.keras.metrics.top_k_categorical_accuracy(y_true, y_pred, k=2)

    def CCELogits(self, y_true, y_pred):
        return tf.keras.backend.categorical_crossentropy(y_true, y_pred, from_logits=True)

    def accuracy(self, y_true, y_pred):
        indices = tf.concat([tf.convert_to_tensor([[i] for i in range(self.batch_size)], dtype=tf.int64),
                             tf.expand_dims(tf.keras.backend.argmax(y_pred, axis=-1), 1)], 1)
        nonzeros = tf.math.divide(tf.math.count_nonzero(tf.gather_nd(y_true, indices)),self.batch_size)
        return nonzeros 

    def percent_error(self, y_true, y_pred):
        return tf.math.multiply(tf.math.divide(tf.math.abs(tf.math.subtract(y_true, y_pred)), tf.math.maximum(y_true, 1)), 100)

    def create(self):
        inputs = tf.keras.Input(shape=(self.feature_shape, ), name="state")
        layer = layers.Dense(512, activation='elu')(inputs)
        layer = layers.Dense(512, activation='elu')(layer)
        layer = layers.Dense(256, activation='elu')(layer)
        layer = layers.Dense(128, activation='elu')(layer)
        layer = layers.Dense(128, activation='elu')(layer)

        policy = layers.Dense(self.policy_shape, activation='linear', name="policy")(layer)
        value = layers.Dense(self.value_shape, name="value")(layer)

        self.model = tf.keras.Model(inputs=inputs, outputs=[policy, value])

        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                #loss='categorical_crossentropy',
                #loss='kld',
                loss = {"policy": self.CCELogits,
                        "value" : 'mse'},
                metrics={"policy": ['categorical_accuracy', self.top_2_accuracy, self.accuracy],
                         "value" : ['mae', self.percent_error]})

    def train(self, iterator, epochs, steps_per_epoch, verbose, class_weight=None):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, 
                                callbacks=[CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name)), write_graph=False, batch_size=self.batch_size), 
                                        self.checkpoint, self.checkpoint_best],
                                class_weight = class_weight)

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        return softmax(self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose))

    def predict_on_batch(self, nn_input):
        prediction = self.model.predict_on_batch(nn_input)
        value = np.ndarray.tolist(np.squeeze(prediction[1]))
        if isinstance(value, float):
           value = [value]
        policy = softmax(prediction[0])
        policy = np.ndarray.tolist(policy)
        return [policy, value]

    def save(self, path):
        tf.keras.models.save_model(self.model, path)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, 
            custom_objects={"top_2_accuracy": self.top_2_accuracy, "CCELogits": self.CCELogits, "accuracy": self.accuracy
                            , "percent_error": self.percent_error})


class RelationsPolicyNetwork(Model):
    def __init__(self, units_features_size, extra_features_size, policy_shape, model_name, batch_size, learning_rate, model_path, create_network=True):
        self.model_name = model_name
        self.model_path = model_path
        self.units_features_size = units_features_size
        self.extra_features_size = extra_features_size
        self.policy_shape = policy_shape
        self.batch_size = batch_size
        self.learning_rate = learning_rate
        self.epochs = 0

        #self.checkpoint_best = tf.keras.callbacks.ModelCheckpoint(self.model_path.split(".")[0] + "_bestCA.h5", monitor='categorical_accuracy', save_best_only=True, mode='max')
        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)
        self.early_stop = tf.keras.callbacks.EarlyStopping(monitor="val_loss", patience=20, mode="min")
        
        if create_network:
            self.create()

    #def exponential_decay(self, epoch, lr):
    #    decay_rate = 0.70
    #    reduce_every_epochs = 1.0
    #    return lr * pow(decay_rate, math.floor((epoch+1) / reduce_every_epochs))

    def top_3_accuracy(self, y_true, y_pred):
        return tf.keras.metrics.top_k_categorical_accuracy(y_true, y_pred, k=3)

    def CCELogits(self, y_true, y_pred):
        return tf.keras.backend.categorical_crossentropy(y_true, y_pred, from_logits=True)

    def accuracy(self, y_true, y_pred):
        indices = tf.concat([tf.convert_to_tensor([[i] for i in range(self.batch_size)], dtype=tf.int64),
                              tf.expand_dims(tf.keras.backend.argmax(y_pred, axis=-1), 1)], 1)
        #indices = tf.keras.backend.argmax(y_pred, axis=-1)
        nonzeros = tf.math.divide(tf.math.count_nonzero(tf.gather_nd(y_true, indices)),self.batch_size)
        return nonzeros 

    def create(self):
        units_output_size = 512
        
        units_input = tf.keras.Input(shape=(None, self.units_features_size), name="units_input")

        layer_units = layers.Dense(1024, activation='elu', name="units_layer1")(units_input)
        layer_units = layers.Dense(512, activation='elu', name="units_layer2")(layer_units)
        layer_units = layers.Dense(512, activation='elu', name="units_layer3")(layer_units)
        units_output = layers.Dense(units_output_size, activation='elu', name="units_output")(layer_units)
        units_output = layers.Lambda(lambda x: tf.keras.backend.mean(x, axis=1), name="average_units_output")(units_output)
        
        extra_features_input = tf.keras.Input(shape=(self.extra_features_size, ), name="extra_features_input")
        concatenate_layer = layers.Concatenate()([units_output, extra_features_input])

        layer = layers.Dense(2048, activation='elu', name="state_layer1")(concatenate_layer)
        layer = layers.Dense(1024, activation='elu', name="state_layer2")(layer)
        layer = layers.Dense(512, activation='elu', name="state_layer3")(layer)
        layer = layers.Dense(512, activation='elu', name="state_layer4")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer5")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer6")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer7")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer8")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer9")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer10")(layer)

        policy = layers.Dense(self.policy_shape, activation='linear', name="policy")(layer)
        
        self.model = tf.keras.Model(inputs=[units_input, extra_features_input], outputs=policy)
        
        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                loss = {"policy" : self.CCELogits},
                metrics= {"policy" : ['categorical_accuracy', self.top_3_accuracy, self.accuracy]})

    def train(self, iterator, epochs, steps_per_epoch, verbose, validation_iterator, validation_steps):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, validation_data=validation_iterator, validation_steps=validation_steps,
                                callbacks=[CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name)), write_graph=False, batch_size=self.batch_size), 
                                        self.checkpoint, self.early_stop]) #self.checkpoint_best])

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        predictions = softmax(self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose), axis=1)
        output = []
        for i in range(len(predictions)):
            output.append(np.ndarray.tolist(predictions[i]))
        return output

    def predict_on_batch(self, nn_input):
        return np.ndarray.tolist(np.squeeze(softmax(self.model.predict_on_batch(nn_input))))

    def save(self, path):
        tf.keras.models.save_model(self.model, path,)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, compile=False,
            custom_objects={"top_3_accuracy": self.top_3_accuracy, "CCELogits": self.CCELogits, "accuracy": self.accuracy})

    def compile(self):
        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                loss = {"policy" : self.CCELogits},
                metrics= {"policy" : ['categorical_accuracy', self.top_3_accuracy, self.accuracy]})

class RelationsValueNetwork(Model):
    def __init__(self, units_features_size, extra_features_size, model_name, batch_size, learning_rate, model_path, create_network=True):
        self.model_name = model_name
        self.model_path = model_path
        self.units_features_size = units_features_size
        self.extra_features_size = extra_features_size
        self.batch_size = batch_size
        self.learning_rate = learning_rate
        self.epochs = 0

        #self.checkpoint_best = tf.keras.callbacks.ModelCheckpoint(self.model_path.split(".")[0] + "_bestCA.h5", monitor='categorical_accuracy', save_best_only=True, mode='max')
        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)
        self.early_stop = tf.keras.callbacks.EarlyStopping(monitor="val_loss", patience=20, mode="min")
        
        if create_network:
            self.create()

    def create(self):
        units_output_size = 512
        
        units_input = tf.keras.Input(shape=(None, self.units_features_size), name="units_input")

        layer_units = layers.Dense(1024, activation='elu', name="units_layer1")(units_input)
        layer_units = layers.Dense(512, activation='elu', name="units_layer2")(layer_units)
        layer_units = layers.Dense(512, activation='elu', name="units_layer3")(layer_units)
        units_output = layers.Dense(units_output_size, activation='elu', name="units_output")(layer_units)
        units_output = layers.Lambda(lambda x: tf.keras.backend.mean(x, axis=1), name="average_units_output")(units_output)
        
        extra_features_input = tf.keras.Input(shape=(self.extra_features_size, ), name="extra_features_input")
        concatenate_layer = layers.Concatenate()([units_output, extra_features_input])

        layer = layers.Dense(2048, activation='elu', name="state_layer1")(concatenate_layer)
        layer = layers.Dense(1024, activation='elu', name="state_layer2")(layer)
        layer = layers.Dense(512, activation='elu', name="state_layer3")(layer)
        layer = layers.Dense(512, activation='elu', name="state_layer4")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer5")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer6")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer7")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer8")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer9")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer10")(layer)

        value = layers.Dense(1, activation='relu', name="value")(layer)
        
        self.model = tf.keras.Model(inputs=[units_input, extra_features_input], outputs=value)
        
        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                loss = 'mae',
                metrics = ['mae'])

    def train(self, iterator, epochs, steps_per_epoch, verbose, validation_iterator, validation_steps):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, validation_data=validation_iterator, validation_steps=validation_steps,
                                callbacks=[CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name)), write_graph=False, batch_size=self.batch_size), 
                                        self.checkpoint, self.early_stop]) #self.checkpoint_best])

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        predictions = softmax(self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose), axis=1)
        output = []
        for i in range(len(predictions)):
            output.append(np.ndarray.tolist(predictions[i]))
        return output

    def predict_on_batch(self, nn_input):
        return np.ndarray.tolist(np.squeeze(softmax(self.model.predict_on_batch(nn_input))))

    def save(self, path):
        tf.keras.models.save_model(self.model, path,)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, compile=False)

    def compile(self):
        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                loss = 'mae',
                metrics = 'mae')                      

class RelationsPolicyAndValueNetwork(Model):
    def __init__(self, units_features_size, extra_features_size, policy_shape, model_name, batch_size, learning_rate, model_path, create_network=True):
        self.model_name = model_name
        self.model_path = model_path
        self.units_features_size = units_features_size
        self.extra_features_size = extra_features_size
        self.policy_shape = policy_shape
        self.batch_size = batch_size
        self.learning_rate = learning_rate
        self.epochs = 0

        #self.checkpoint_best = tf.keras.callbacks.ModelCheckpoint(self.model_path.split(".")[0] + "_bestCA.h5", monitor='categorical_accuracy', save_best_only=True, mode='max')
        self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)
        self.early_stop = tf.keras.callbacks.EarlyStopping(monitor="val_loss", patience=20, mode="min")
        
        if create_network:
            self.create()

    #def exponential_decay(self, epoch, lr):
    #    decay_rate = 0.70
    #    reduce_every_epochs = 1.0
    #    return lr * pow(decay_rate, math.floor((epoch+1) / reduce_every_epochs))

    def top_3_accuracy(self, y_true, y_pred):
        return tf.keras.metrics.top_k_categorical_accuracy(y_true, y_pred, k=3)

    def CCELogits(self, y_true, y_pred):
        return tf.keras.backend.categorical_crossentropy(y_true, y_pred, from_logits=True)

    def accuracy(self, y_true, y_pred):
        indices = tf.concat([tf.convert_to_tensor([[i] for i in range(self.batch_size)], dtype=tf.int64),
                              tf.expand_dims(tf.keras.backend.argmax(y_pred, axis=-1), 1)], 1)
        #indices = tf.keras.backend.argmax(y_pred, axis=-1)
        nonzeros = tf.math.divide(tf.math.count_nonzero(tf.gather_nd(y_true, indices)),self.batch_size)
        return nonzeros 

    def MAEWithScalar(self, y_true, y_pred):
        return tf.math.multiply(tf.constant(10.0, shape=(self.batch_size,)), tf.keras.losses.MAE(y_true, y_pred))

    def create(self):
        units_output_size = 512
        
        units_input = tf.keras.Input(shape=(None, self.units_features_size), name="units_input")

        layer_units = layers.Dense(1024, activation='elu', name="units_layer1")(units_input)
        layer_units = layers.Dense(512, activation='elu', name="units_layer2")(layer_units)
        layer_units = layers.Dense(512, activation='elu', name="units_layer3")(layer_units)
        units_output = layers.Dense(units_output_size, activation='elu', name="units_output")(layer_units)
        units_output = layers.Lambda(lambda x: tf.keras.backend.mean(x, axis=1), name="average_units_output")(units_output)
        
        extra_features_input = tf.keras.Input(shape=(self.extra_features_size, ), name="extra_features_input")
        concatenate_layer = layers.Concatenate()([units_output, extra_features_input])

        layer = layers.Dense(2048, activation='elu', name="state_layer1")(concatenate_layer)
        layer = layers.Dense(1024, activation='elu', name="state_layer2")(layer)
        layer = layers.Dense(512, activation='elu', name="state_layer3")(layer)
        layer = layers.Dense(512, activation='elu', name="state_layer4")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer5")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer6")(layer)
        layer = layers.Dense(256, activation='elu', name="state_layer7")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer8")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer9")(layer)
        layer = layers.Dense(128, activation='elu', name="state_layer10")(layer)

        policy_layer = layers.Dense(128, activation='elu', name="policy_layer1")(layer)
        policy_layer = layers.Dense(128, activation='elu', name="policy_layer2")(policy_layer)
        policy = layers.Dense(self.policy_shape, activation='linear', name="policy")(policy_layer)

        value_layer = layers.Dense(64, activation='elu', name="value_layer1")(layer)
        value_layer = layers.Dense(64, activation='elu', name="value_layer2")(value_layer)
        value = layers.Dense(1, activation='relu', name="value")(value_layer)
        
        self.model = tf.keras.Model(inputs=[units_input, extra_features_input], outputs=[policy, value])
        
        #self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                loss = {"policy" : self.CCELogits,
                            "value" : self.MAEWithScalar},
                metrics= {"policy" : ['categorical_accuracy', self.top_3_accuracy, self.accuracy],
                          "value" : ['mae']})

    def train(self, iterator, epochs, steps_per_epoch, verbose, validation_iterator, validation_steps):
        return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, validation_data=validation_iterator, validation_steps=validation_steps,
                                callbacks=[CustomTensorBoard(self.model, log_dir=os.path.join(os.getcwd(), os.path.join("logs", self.model_name)), write_graph=False, batch_size=self.batch_size), 
                                        self.checkpoint, self.early_stop]) #self.checkpoint_best])

    def evaluate(self, iterator, steps, verbose):
        return self.model.evaluate(iterator, steps=steps, verbose=verbose)

    def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
        prediction = self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose)
        softmaxed_values = softmax(prediction[0], axis=1)
        output = []
        for i in range(len(prediction[0])):
            output.append([np.ndarray.tolist(softmaxed_values[i]), np.ndarray.tolist(prediction[1][i])])
        return output

    def predict_on_batch(self, nn_input):
        return np.ndarray.tolist(np.squeeze(softmax(self.model.predict_on_batch(nn_input))))

    def save(self, path):
        tf.keras.models.save_model(self.model, path,)

    def load(self, path):
        self.model = tf.keras.models.load_model(path, compile=False,
            custom_objects={"top_3_accuracy": self.top_3_accuracy, "CCELogits": self.CCELogits, "accuracy": self.accuracy
                            , "MAEWithScalar": self.MAEWithScalar})

    def compile(self):
        self.model.compile(optimizer=tf.keras.optimizers.Adam(self.learning_rate),
                loss = {"policy" : self.CCELogits,
                            "value" : self.MAEWithScalar},
                metrics= {"policy" : ['categorical_accuracy', self.top_3_accuracy, self.accuracy],
                          "value" : ['mae']})
                        