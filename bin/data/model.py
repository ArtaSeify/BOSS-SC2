import tensorflow as tf
from tensorflow.keras import layers
import os

class Model():
	def __init__(self):
		return

class IntegralValueNN(Model):
	def __init__(self, input_shape, output_shape, model_name, batch_size, learning_rate=1e-3):

		# creating network
		inputs = tf.keras.Input(shape=(input_shape, ))
		layer = layers.Dense(2048, activation='elu')(inputs)
		layer = layers.Dense(1024, activation='elu')(layer)
		layer = layers.Dense(1024, activation='elu')(layer)
		layer = layers.Dense(512, activation='elu')(layer)
		layer = layers.Dense(512, activation='elu')(layer)
		layer = layers.Dense(512, activation='elu')(layer)
		prediction = layers.Dense(output_shape)(layer)

		self.tensorboard = tf.keras.callbacks.TensorBoard(log_dir=os.path.join(os.getcwd(), os.path.join("logs", model_name))
														, write_graph=False, batch_size=batch_size)

		self.model = tf.keras.Model(inputs=inputs, outputs=prediction)

		self.model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate),
              	loss='mse',
              	metrics=['mae'])

	def train(self, iterator, epochs, steps_per_epoch, verbose):
		return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, callbacks=[self.tensorboard])

	def evaluate(self, iterator, steps, verbose):
		return self.model.evaluate(iterator, steps=steps, verbose=verbose)

	def predict(self, nn_input, steps, verbose):
		print(nn_input)
		return self.model.predict(nn_input, steps=steps, verbose=verbose)

	def save(self, file_name):
		tf.keras.models.save_model(self.model, os.path.join(os.getcwd(), os.path.join("models", file_name)))

	def load(self, file_name):
		self.model = tf.keras.models.load_model(os.path.join(os.getcwd(), os.path.join("models", file_name)))