import tensorflow as tf
import json

class DataLoader():
    def __init__(self, units_shape, extra_feat_shape, policy_shape, value_shape, train_samples, test_samples, policy_and_value=False, shuffle=True, batch_size=32, workers=4):
        self.file_list = tf.keras.backend.placeholder(dtype=tf.string, shape=[None])
        self.units_shape = units_shape
        self.extra_feat_shape = extra_feat_shape
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
        self.dataset = self.dataset.padded_batch(self.batch_size, (([None, self.units_shape], [self.extra_feat_shape]), [self.policy_shape]))
        #self.dataset = self.dataset.batch(self.batch_size)

        print(self.dataset)

    def _parse_fn_policy(self, csv_string):
        split_string = tf.sparse_tensor_to_dense(tf.string_split(tf.expand_dims(csv_string, axis=0), ","), default_value='')
        split_string_unit_features = split_string[0][:-(self.policy_shape+self.extra_feat_shape)]
        units_shape = [tf.cast(tf.divide(tf.size(split_string_unit_features), self.units_shape), tf.int32), self.units_shape]
        split_string_unit_features = tf.reshape(split_string_unit_features, units_shape)
        split_string_extra_features = split_string[0][-(self.policy_shape+self.extra_feat_shape):-self.policy_shape]
        split_string_policy = split_string[0][-self.policy_shape:]

        units = tf.squeeze(tf.string_to_number(split_string_unit_features, tf.float32))
        extra_features = tf.squeeze(tf.string_to_number(split_string_extra_features, tf.float32))
        policy = tf.squeeze(tf.string_to_number(split_string_policy, tf.float32))

        units.set_shape([None, self.units_shape])
        extra_features.set_shape(self.extra_feat_shape,)
        policy.set_shape(self.policy_shape,)

        return (units, extra_features), policy

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