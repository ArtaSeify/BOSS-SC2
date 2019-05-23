import tensorflow as tf
from data_loader import IntegralPredictionDataset

"""
TODO: READ PARAMETERS FROM JSON FILE
"""

batch_size = 32
cpu_workers = 8
gzip = False

learning_rate = 1e-4
input_shape = 223

is_training = tf.placeholder(tf.bool, name="is_training")

dataset = IntegralPredictionDataset(batch_size=batch_size,
	workers=cpu_workers, gzip=gzip)

(x_data, y_data) = dataset.get_next(is_training)

model = IntegralPredictionNet(x_data, y_data, input_shape, learning_rate)

model.dataset = dataset
# file names of train_set and test_set
model.train_set = model_def["train"]
model.test_set = model_def["test"]
model.max_epochs = 50

results_file = os.path.join(os.getcwd(), "train.out")

with tf.Session() as sess:
	sess.run(tf.global_variables_initializer())

	# for saving variables every epoch
	saver = tf.train.Saver()

	util.log(results_file, "*** Training {} ***\n".format("Integral prediction"))

	prev_test_loss = np.inf
	for epoch in range(model.max_epochs):
		model.dataset.refresh_train(sess, model.train_set)
		feed_dict = {model.keep_prob: 0.8, is_training: True}
		