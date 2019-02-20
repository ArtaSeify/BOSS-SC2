import csv
import os
import glob
import subprocess
import shutil
import numpy as np
from random import randint
import pandas as pd
import tensorflow as tf


'''
Naively one-hot encode samples by assuming values lie in [0,m]
'''
def one_hot_encode(samples, m):
  n = len(samples)
  tmp = samples.astype(np.float32).astype(np.int)
  one_hot = np.zeros((n, m), dtype=np.int)
  one_hot[np.arange(n), tmp] = 1
  return one_hot

'''
Extract the labels, legal actions and features from the dataset.
'''
def separate_labels(data, start, end):
  y = data[start:end,-32:]
  l = data[start:end,-64:-32]
  x = data[start:end,:-64]
  return x, y, l


class Dataset:
  def __init__(self, file_name):
    self.file_name = file_name
    self.num_labels = 32
    self.batch_queue = deque(maxlen=100)
    self.done = False

  ''' 
  Shuffle the dataset so that the batches are generated in random order
  '''
  def shuffle(self):
    # split using number of lines into manageable files
    tmp_path = os.path.join(os.path.split(self.file_name)[0], "tmp")
    if os.path.exists(tmp_path):
      shutil.rmtree(tmp_path)
    os.mkdir(tmp_path)
    splitSize = randint(500000, 1500000)
    subprocess.call(['split', '-dl', str(splitSize), self.file_name, tmp_path + "/a"])
    
    # shuf each file
    for fn in os.listdir(tmp_path):
      fname = os.path.join(tmp_path, fn)
      with open(fname + "_shuffed", 'w') as f:
        subprocess.call(['shuf', fname], stdout=f)
      os.remove(fname)
    
    # concatenate the files back into old file and delete temp files
    os.system("cat " + tmp_path + "/* > " + self.file_name)
    shutil.rmtree(tmp_path)

  '''
  Generator function to yield x and y batches of chosen size
  '''
  def get_next_batch(self, batch_size):
    for batches in pd.read_csv(self.file_name, chunksize=batch_size*1000, header=None):
      m_batches = batches.as_matrix()
      batch_start = 0
      while batch_start + batch_size <= m_batches.shape[0]:
        x, y, l = separate_labels(m_batches, batch_start, batch_start + batch_size)
        batch_start += batch_size
        yield x, y, l


class DatasetInference:
  def __init__(self, file_name, output_shape, input_shape):
    self.file_name = file_name
    self.num_labels = output_shape
    self.num_features = input_shape

  ''' 
  Shuffle the dataset so that the batches are generated in random order
  '''

  def shuffle(self):
    # split using number of lines into manageable files
    tmp_path = os.path.join(os.path.split(self.file_name)[0], "tmp")
    if os.path.exists(tmp_path):
      shutil.rmtree(tmp_path)
    os.mkdir(tmp_path)
    splitSize = randint(500000, 1500000)
    subprocess.call(['split', '-dl', str(splitSize), self.file_name, tmp_path + "/a"])

    # shuf each file
    for fn in os.listdir(tmp_path):
      fname = os.path.join(tmp_path, fn)
      with open(fname + "_shuffed", 'w') as f:
        subprocess.call(['shuf', fname], stdout=f)
      os.remove(fname)

    # concatenate the files back into old file and delete temp files
    os.system("cat " + tmp_path + "/* > " + self.file_name)
    shutil.rmtree(tmp_path)


  def get_next_batch(self, batch_size):
    for batch in pd.read_csv(self.file_name, chunksize=batch_size, header=None):
      x, y= self.separate_data(batch.as_matrix())
      y_onehot = one_hot_encode(y,self.num_labels)
      # add legal moves, for now everything is legal
      l = np.ones((y.shape[0],self.num_labels))
      yield x, y_onehot, l

  def separate_data(self, data):
    y = data[:,-1]
    x = data[:,:-2]
    return x, y

 


'''
Parse the state, action, reward tuple from a 2d numpy array

Assumes the state comes first, followed by legal moves vector (size 32),
followed by one hot encoded action (size 32)
and then the floating point reward.

'''
def parse_SAR(data, batch_start, batch_end):
  is_terminal = data[batch_start:batch_end,-1]
  reward = data[batch_start:batch_end,-2]
  action = data[batch_start:batch_end,-34:-2]
  legal_moves = data[batch_start:batch_end,-66:-34]
  state = data[batch_start:batch_end,:-66]

  return state, action, reward, legal_moves, is_terminal


BATCH_RL_DATA_PATH = "/common/mburo/skat.data/nn/training/batch-rl/"
'''
Dataset for self-play. Uses a full directory of files to simplify logging 
for java tournament
'''
class BatchRLDataset:
  
  def __init__(self, contexts, directory=BATCH_RL_DATA_PATH):
    self._dir = directory
    # for now just add all csv files to a list
    self._used_files = []
    self._df = None
    for root, dirs, files in os.walk(directory):
      
      for f in files:
        matches = True
        for context in contexts:
          if context.upper() not in f.upper():
            matches = False
        
        if matches:
          fName = os.path.join(root, f)
          self._used_files.append(fName)
          self._df = pd.concat([self._df, pd.read_csv(fName, header=None)])


  def clean_files(self):
    for f in self._used_files:
      os.remove(f)
    self._used_files = []
  

  def has_batch(self, batch_size):
    return self._df is not None and self._df.shape[0] >= batch_size


  def get_batch(self, batch_size):
    if not self.has_batch(batch_size):
      return
    
    batch_start = 0
    matrix = self._df.as_matrix()
    while batch_start + batch_size <= matrix.shape[0]:
      state, action, reward, legal_actions, is_term = parse_SAR(matrix, 
          batch_start, batch_start + batch_size)
      batch_start += batch_size
      yield state, action, reward, legal_actions, is_term



'''
Wrapper for tensorflow's dataset API
'''
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