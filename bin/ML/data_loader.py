import tensorflow as tf
import json

class DataLoader():
    def __init__(self, feat_shape, policy_shape, value_shape, train_samples, test_samples, policy_and_value=False, shuffle=True, batch_size=32, workers=4):
        self.file_list = tf.keras.backend.placeholder(dtype=tf.string, shape=[None])
        self.feat_shape = feat_shape
        self.policy_shape = policy_shape
        self.value_shape = value_shape
        self.batch_size = batch_size
        self.twoHeads = policy_and_value

        self.train_samples = train_samples
        self.test_samples = test_samples
        
        self.dataset = tf.data.TextLineDataset(self.file_list)
        self.dataset = self.dataset.repeat()
        self.dataset = self.dataset.map(self._parse_fn_policy if value_shape == 0 else self._parse_fn, num_parallel_calls=workers)
        if shuffle:
            self.dataset = self.dataset.shuffle(buffer_size=min(max(train_samples, test_samples), 100000))
        self.dataset = self.dataset.prefetch(self.batch_size*100)
        self.dataset = self.dataset.batch(self.batch_size)

    def _parse_fn_policy(self, csv_string):
        split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
        data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
        d = tf.squeeze(data)

        x, policy = d[:-self.policy_shape], d[-self.policy_shape:]
        # adds zeroes for missing units
        x = tf.concat([x, tf.zeros(self.feat_shape - tf.size(x))], 0)

        x.set_shape(self.feat_shape,)
        policy.set_shape(self.policy_shape,)

        return x, policy      

    def _parse_fn(self, csv_string):
        split_string = tf.string_split(tf.expand_dims(csv_string, axis=0), ",")
        data = tf.string_to_number(tf.sparse_tensor_to_dense(split_string, default_value=''), tf.float32)
        d = tf.squeeze(data)
        x, policy, value = d[:-(self.policy_shape+self.value_shape)], d[-(self.policy_shape+self.value_shape):-self.value_shape], d[-self.value_shape:]
        x.set_shape(self.feat_shape,)
        policy.set_shape(self.policy_shape,)
        value.set_shape(self.value_shape,)

        if self.twoHeads:
            return x, (policy, value)
        else:
            return x, policy       

    def make_iterator(self, sess, file_list):
        # if "test" in file_list[0]:
        #     self.dataset = self.dataset.take(self.test_samples)
        # else:
        #     self.dataset = self.dataset.take(self.train_samples)

        self.iterator = self.dataset.make_initializable_iterator()
        sess.run(self.iterator.initializer, feed_dict={self.file_list: file_list})

        return self.iterator