import tensorflow as tf
from data_filter_functions import createUnitDict
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

        self.unit_dict = createUnitDict("ActionData.txt")

        with open("../SC2Data.json") as sc2dataFile:
            sc2data = json.load(sc2dataFile)
            self.mins_per_worker_per_sec = sc2data["MineralsPerWorkerPerFrame"]
            self.gas_per_worker_per_sec = sc2data["GasPerWorkerPerFrame"]
        
        self.dataset = tf.data.TextLineDataset(self.file_list)
        self.dataset = self.dataset.repeat()
        self.dataset = self.dataset.map(self._parse_fn, num_parallel_calls=workers)
        if shuffle:
            self.dataset = self.dataset.shuffle(buffer_size=min(max(train_samples, test_samples), 100000))
        self.dataset = self.dataset.prefetch(self.batch_size*100)
        self.dataset = self.dataset.batch(self.batch_size)

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