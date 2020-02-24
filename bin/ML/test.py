from __future__ import print_function
import tensorflow as tf
from tensorflow.keras import layers
from keras.datasets import cifar10
from keras.preprocessing.image import ImageDataGenerator
from keras.models import Sequential
from keras.layers import Dense, Dropout, Activation, Flatten
from keras.layers import Conv2D, MaxPooling2D
import os

config = tf.ConfigProto( device_count = {'GPU': 0})
sess = tf.Session(config=config)
tf.keras.backend.set_session(sess)

batch_size = 32
num_classes = 10
epochs = 100
data_augmentation = False
num_predictions = 20
save_dir = os.path.join(os.getcwd(), 'saved_models')
model_name = 'keras_cifar10_trained_model.h5'

# The data, split between train and test sets:
(x_train, y_train), (x_test, y_test) = cifar10.load_data()
print('x_train shape:', x_train.shape)
print(x_train.shape[0], 'train samples')
print(x_test.shape[0], 'test samples')

# Convert class vectors to binary class matrices.
y_train = tf.keras.utils.to_categorical(y_train, num_classes)
y_test = tf.keras.utils.to_categorical(y_test, num_classes)

inputs = tf.keras.Input(shape=x_train.shape[1:])
layer = layers.Flatten()(inputs)
layer = layers.Dense(64, activation='elu')(layer)
layer = layers.Dense(64, activation='elu')(layer)
layer = layers.Dense(32, activation='elu')(layer)
layer = layers.Dense(32, activation='elu')(layer)
prediction = layers.Dense(num_classes, activation='softmax')(layer)

model = tf.keras.Model(inputs=inputs, outputs=prediction)

#self.lrs = tf.keras.callbacks.LearningRateScheduler(self.exponential_decay)

model.compile(optimizer=tf.keras.optimizers.Adam(1e-4),
        #loss='categorical_crossentropy',
        #loss='kld',
        loss = 'categorical_crossentropy',
        metrics=['categorical_accuracy'])

x_train = x_train.astype('float32')
x_test = x_test.astype('float32')
x_train /= 255
x_test /= 255

if not data_augmentation:
    print('Not using data augmentation.')
    model.fit(x_train, y_train,
              batch_size=batch_size,
              epochs=epochs,
              validation_data=(x_test, y_test),
              shuffle=True)

# Score trained model.
scores = model.evaluate(x_test, y_test, verbose=1)
print('Test loss:', scores[0])
print('Test accuracy:', scores[1])