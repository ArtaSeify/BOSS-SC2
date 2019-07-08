import tensorflow as tf
import json
from math import ceil, floor

class DataLoader():
    def __init__(self, units_shape, extra_feat_shape, policy_shape, value_shape, samples, output_type, shuffle=True, batch_size=32, workers=4):
        self.file_list = tf.keras.backend.placeholder(dtype=tf.string, shape=[None])
        self.units_shape = units_shape
        self.extra_feat_shape = extra_feat_shape
        self.policy_shape = policy_shape
        self.value_shape = value_shape
        self.batch_size = batch_size

        self.dataset = tf.data.TextLineDataset(self.file_list)
        self.dataset = self.dataset.repeat()
        if output_type == "P":
            self.dataset = self.dataset.map(self._parse_fn_policy, num_parallel_calls=workers)
        elif output_type == "V":
            self.dataset = self.dataset.map(self._parse_fn_value, num_parallel_calls=workers)
        elif output_type == "B":
            self.dataset = self.dataset.map(self._parse_fn_policy_value, num_parallel_calls=workers)
        else:
            assert False
        
        if shuffle:
            self.dataset = self.dataset.shuffle(buffer_size=min(samples, 100000))
        self.dataset = self.dataset.prefetch(self.batch_size*10)

        if output_type == "P":
            self.dataset = self.dataset.padded_batch(self.batch_size, (([None, self.units_shape], [self.extra_feat_shape]), [self.policy_shape]))
        elif output_type == "V":
            self.dataset = self.dataset.padded_batch(self.batch_size, (([None, self.units_shape], [self.extra_feat_shape]), [self.value_shape]))
        elif output_type == "B":
            self.dataset = self.dataset.padded_batch(self.batch_size, (([None, self.units_shape], [self.extra_feat_shape]), ([self.policy_shape], [self.value_shape])))
        else:
            assert False

    def _parse_fn_policy(self, csv_string):
        split_string = tf.sparse_tensor_to_dense(tf.string_split(tf.expand_dims(csv_string, axis=0), ","), default_value='')
        split_string_unit_features = split_string[0][:-(self.policy_shape+self.extra_feat_shape+self.value_shape)]
        num_units = tf.cast(tf.divide(tf.size(split_string_unit_features), self.units_shape), tf.int32)

        split_string_unit_features = tf.cond(tf.equal(num_units, 0), 
                                             true_fn=lambda:tf.constant("0", shape=[1, self.units_shape], dtype=tf.string), 
                                             false_fn=lambda:tf.reshape(split_string_unit_features, [num_units, self.units_shape]))
        split_string_extra_features = split_string[0][-(self.policy_shape+self.extra_feat_shape+self.value_shape):-(self.policy_shape+self.value_shape)]
        split_string_policy = split_string[0][-(self.policy_shape+self.value_shape):-self.value_shape]
        split_string_value = split_string[0][-(self.value_shape):]

        units = tf.string_to_number(split_string_unit_features, tf.float32)
        units = tf.cond(tf.less_equal(num_units, 1), true_fn=lambda:units, false_fn=lambda:tf.squeeze(units))    
        extra_features = tf.squeeze(tf.string_to_number(split_string_extra_features, tf.float32))
        policy = tf.squeeze(tf.string_to_number(split_string_policy, tf.float32))
        value = tf.string_to_number(split_string_value, tf.float32)

        units.set_shape([None, self.units_shape])
        extra_features.set_shape(self.extra_feat_shape,)
        policy.set_shape(self.policy_shape,)
        value.set_shape(self.value_shape,)

        return (units, extra_features), policy 

    def _parse_fn_value(self, csv_string):
        split_string = tf.sparse_tensor_to_dense(tf.string_split(tf.expand_dims(csv_string, axis=0), ","), default_value='')
        split_string_unit_features = split_string[0][:-(self.policy_shape+self.extra_feat_shape+self.value_shape)]
        num_units = tf.cast(tf.divide(tf.size(split_string_unit_features), self.units_shape), tf.int32)

        split_string_unit_features = tf.cond(tf.equal(num_units, 0), 
                                             true_fn=lambda:tf.constant("0", shape=[1, self.units_shape], dtype=tf.string), 
                                             false_fn=lambda:tf.reshape(split_string_unit_features, [num_units, self.units_shape]))
        split_string_extra_features = split_string[0][-(self.policy_shape+self.extra_feat_shape+self.value_shape):-(self.policy_shape+self.value_shape)]
        split_string_policy = split_string[0][-(self.policy_shape+self.value_shape):-self.value_shape]
        split_string_value = split_string[0][-(self.value_shape):]

        units = tf.string_to_number(split_string_unit_features, tf.float32)
        units = tf.cond(tf.less_equal(num_units, 1), true_fn=lambda:units, false_fn=lambda:tf.squeeze(units))    
        extra_features = tf.squeeze(tf.string_to_number(split_string_extra_features, tf.float32))
        policy = tf.squeeze(tf.string_to_number(split_string_policy, tf.float32))
        value = tf.string_to_number(split_string_value, tf.float32)

        units.set_shape([None, self.units_shape])
        extra_features.set_shape(self.extra_feat_shape,)
        policy.set_shape(self.policy_shape,)
        value.set_shape(self.value_shape,)

        return (units, extra_features), value

    def _parse_fn_policy_value(self, csv_string):
        split_string = tf.sparse_tensor_to_dense(tf.string_split(tf.expand_dims(csv_string, axis=0), ","), default_value='')
        split_string_unit_features = split_string[0][:-(self.policy_shape+self.extra_feat_shape+self.value_shape)]
        num_units = tf.cast(tf.divide(tf.size(split_string_unit_features), self.units_shape), tf.int32)

        split_string_unit_features = tf.cond(tf.equal(num_units, 0), 
                                             true_fn=lambda:tf.constant("0", shape=[1, self.units_shape], dtype=tf.string), 
                                             false_fn=lambda:tf.reshape(split_string_unit_features, [num_units, self.units_shape]))
        split_string_extra_features = split_string[0][-(self.policy_shape+self.extra_feat_shape+self.value_shape):-(self.policy_shape+self.value_shape)]
        split_string_policy = split_string[0][-(self.policy_shape+self.value_shape):-self.value_shape]
        split_string_value = split_string[0][-(self.value_shape):]

        units = tf.string_to_number(split_string_unit_features, tf.float32)
        units = tf.cond(tf.less_equal(num_units, 1), true_fn=lambda:units, false_fn=lambda:tf.squeeze(units))    
        extra_features = tf.squeeze(tf.string_to_number(split_string_extra_features, tf.float32))
        policy = tf.squeeze(tf.string_to_number(split_string_policy, tf.float32))
        value = tf.string_to_number(split_string_value, tf.float32)

        units.set_shape([None, self.units_shape])
        extra_features.set_shape(self.extra_feat_shape,)
        policy.set_shape(self.policy_shape,)
        value.set_shape(self.value_shape,)

        return (units, extra_features), (policy, value) 

    def make_iterator(self, sess, file_list):

        self.iterator = self.dataset.make_initializable_iterator()
        sess.run(self.iterator.initializer, feed_dict={self.file_list: file_list})

        return self.iterator