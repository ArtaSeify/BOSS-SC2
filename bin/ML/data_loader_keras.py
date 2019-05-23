class TFDataset():
  def __init__(self, batch_size=32, workers=4, gzip=False):
    self.file_list = tf.placeholder(tf.string, shape=[None])
    self.batch_size = batch_size
    if gzip:
      self.dataset = tf.data.TextLineDataset(self.file_list, compression_type="GZIP")
    else:
      self.dataset = tf.data.TextLineDataset(self.file_list)
    
    self.dataset = self.dataset.map(self._parse_fn, num_parallel_calls=workers)
    self.dataset = self.dataset.shuffle(buffer_size=5000)
    self.dataset = self.dataset.prefetch(batch_size*100)
    self.dataset = self.dataset.batch(self.batch_size)
    self.train_set_iterator = self.dataset.make_initializable_iterator()
    self.test_set_iterator = self.dataset.make_initializable_iterator()

  '''
  Parse each line from csv to tensor
  '''
  def _parse_fn(self, csv_string):
    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
    data = tf.string_to_number(
        tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
    data = tf.squeeze(data)
    x_data,legal_data, y_data = data[:-64], data[-64:-32], data[-32:]
    return x_data, y_data, legal_data

  '''
  Tell this dataset to use this set of filenames as a data source
  for the next epoch
  '''
  def refresh_train(self, sess, file_names):
    return sess.run(self.train_set_iterator.initializer,
        feed_dict={self.file_list: file_names})

  def refresh_test(self, sess, file_names):
    return sess.run(self.test_set_iterator.initializer,
        feed_dict={self.file_list: file_names})

  def get_next(self, is_training):
    return tf.cond(is_training, self.get_next_train, self.get_next_test)

  def get_next_train(self):
    return self.train_set_iterator.get_next()

  def get_next_test(self):
    return self.test_set_iterator.get_next()

  

class InferenceDataset(TFDataset):
  def __init__(self, batch_size=32, workers=4, gzip=False):
    super().__init__(batch_size, workers, gzip)


  def _parse_fn(self, csv_string):
    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
    d = tf.squeeze(data)
    (xd, hd, sc) = d[:-1088], d[-1088:-128], d[-128:]
    return xd, hd, sc


class InferenceBaselineDataset(TFDataset):
  def __init__(self, batch_size=32, workers=4, gzip=False):
    super().__init__(batch_size, workers, gzip)


  def _parse_fn(self, csv_string):
    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
    d = tf.squeeze(data)
    (xd, sc) = d[:-128], d[-128:]
    return xd, sc


class IntegralPredictionDataset(TFDataset):
  def __init__(self, batch_size=32, workers=4, gzip=False):
    super().__init__(batch_size, workers, gzip)

  def _parse_fn(self, csv_string):
    split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
    data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
    d = tf.squeeze(data)
    (x, y) = d[-223:-1], d[-1:]
    return x, y