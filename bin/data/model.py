import tensorflow as tf
from tensorflow.keras import layers
import os

class Model():
	def __init__(self):
		return

class IntegralValueNN(Model):
	def __init__(self, input_shape, output_shape, model_name, batch_size, learning_rate, model_path, create_network=True):
		self.model_path = model_path

		if create_network:
			self.create(input_shape, output_shape, model_name, batch_size, learning_rate)
		else:
			self.tensorboard = tf.keras.callbacks.TensorBoard(log_dir=os.path.join(os.getcwd(), os.path.join("logs", model_name))
														, write_graph=False, batch_size=batch_size)

	def percent_error(self, y_true, y_pred):
		if y_true == 0:
			return tf.math.divide(tf.keras.backend.abs(tf.math.subtract(y_true, y_pred)), 1)
		return tf.math.divide(tf.keras.backend.abs(tf.math.subtract(y_true, y_pred)), y_true)

	def create(self, input_shape, output_shape, model_name, batch_size, learning_rate=1e-3):
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

		self.checkpoint = tf.keras.callbacks.ModelCheckpoint(self.model_path)

		self.model = tf.keras.Model(inputs=inputs, outputs=prediction)

		self.model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate),
              	loss='msle',
              	metrics=['mae', 'MAPE'])

	def train(self, iterator, epochs, steps_per_epoch, verbose):
		return self.model.fit(iterator, epochs=epochs, steps_per_epoch=steps_per_epoch, verbose=verbose, callbacks=[self.tensorboard, self.checkpoint])

	def evaluate(self, iterator, steps, verbose):
		return self.model.evaluate(iterator, steps=steps, verbose=verbose)

	def predict(self, nn_input, batch_size=None, steps=1, verbose=0):
		return self.model.predict(nn_input, batch_size=batch_size, steps=steps, verbose=verbose)

	def predict_on_batch(self, nn_input):
		return self.model.predict_on_batch(nn_input)

	def save(self, path):
		tf.keras.models.save_model(self.model, path)

	def load(self, path):
		#, custom_objects={"percent_error": self.percent_error}
		self.model = tf.keras.models.load_model(path)