import tensorflow as tf
from tensorflow.keras import layers

def tfdata_generator(images, labels, is_training, batch_size=128):
	'''Construct a data generator using `tf.Dataset`. '''

	def map_fn(image, label):
		'''Preprocess raw data to trainable input. '''
		x = tf.reshape(tf.cast(image, tf.float32), (28, 28, 1))
		y = tf.one_hot(tf.cast(label, tf.uint8), 5)
		return x, y

	dataset = tf.data.Dataset.from_tensor_slices((images, labels))
	print(dataset)

	if is_training:
		dataset = dataset.shuffle(1000)  # depends on sample size
	dataset = dataset.map(map_fn)
	dataset = dataset.batch(batch_size)
	dataset = dataset.prefetch(tf.contrib.data.AUTOTUNE)
	dataset = dataset.repeat()

	return dataset

# Load mnist training data
(x_train, y_train), _ = tf.keras.datasets.mnist.load_data()
training_set = tfdata_generator(x_train, y_train,is_training=True)

print(x_train)
print(y_train)
print(training_set)

inputs = tf.keras.Input(shape=(28, 28, 1))
layer = layers.Flatten()(inputs)
layer = layers.Dense(64, activation='relu')(layer)
layer = layers.Dense(64, activation='relu')(layer)
prediction = layers.Dense(5, activation='softmax')(layer)

model = tf.keras.Model(inputs=inputs, outputs=prediction)

model.compile(optimizer=tf.train.AdamOptimizer(0.001),
              loss='categorical_crossentropy',
              metrics=['accuracy'])          
model.fit(
    training_set.make_one_shot_iterator(),
    steps_per_epoch=len(x_train) // 128,
    epochs=5,
    verbose = 1)


