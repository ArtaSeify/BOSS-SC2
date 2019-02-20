import tensorflow as tf

class DataLoader():
	def __init__(self, feat_shape, pred_shape, batch_size=32, workers=4):
		self.file_list = tf.keras.backend.placeholder(dtype=tf.string, shape=[None])
		self.feat_shape = feat_shape
		self.pred_shape = pred_shape
		self.batch_size = batch_size

		self.dataset = tf.data.TextLineDataset(self.file_list)
		self.dataset = self.dataset.map(self._parse_fn, num_parallel_calls=workers)
		self.dataset = self.dataset.shuffle(buffer_size=5000)
		self.dataset = self.dataset.prefetch(self.batch_size*100)
		self.dataset = self.dataset.batch(self.batch_size)
		self.dataset = self.dataset.repeat()

	def _parse_fn(self, csv_string):
	    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
	    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
	    d = tf.squeeze(data)
	    x, y = d[-(self.feat_shape+1):-self.pred_shape], d[-self.pred_shape:]
	    x.set_shape(self.feat_shape,)
	    y.set_shape(self.pred_shape,)
	    return x, y

	def make_iterator(self, sess, file_list):
		self.iterator = self.dataset.make_initializable_iterator()
		sess.run(self.iterator.initializer, feed_dict={self.file_list: file_list})

		return self.iterator