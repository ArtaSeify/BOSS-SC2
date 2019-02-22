import tensorflow as tf
import argparse
import numpy as np
import os
import sys
import json
import util
from models import InferenceFCNet, InferenceLinear
from data_loader import InferenceDataset, InferenceBaselineDataset 
from constants import VARIABLES_EXT


parser = argparse.ArgumentParser(description='Test a model')
parser.add_argument('config_file', help='config file for training')
parser.add_argument('output_dir', help='output location for saving models')
parser.add_argument('model_name', help='name for saved model')
parser.add_argument('--baseline', dest='baseline', action='store_true',
    help='train baseline (no cardplay features)')
parser.add_argument('--linear', dest='linear', action='store_true',
    help='train linear model')
parser.add_argument('--hist', dest='hist', action='store_true',
    help='use history module')
parser.add_argument('--sinkhorn', dest='sink', action='store_true',
    help='apply sinkhorn iteratively to outputs')

args = parser.parse_args()

with open(args.config_file) as config_file:
  config = json.load(config_file)

batch_size = config["batch_size"]
gzip = config["gzipped_data"]
cpu_workers = config["cpu_workers"]

# define dataset, input tensors and model
is_training = tf.placeholder(tf.bool, name="is_training")

model_defs = config["models"]
models = []

for model_def in model_defs:
  # baseline just means no hist in this case, but it can be any number of 
  # features
  if args.baseline or not args.hist:
    dataset = InferenceBaselineDataset(batch_size=batch_size,
        workers=cpu_workers, gzip=gzip)
    (x_data, single_card_data)  = dataset.get_next(is_training)
    hist_data = None
  else:
    dataset = InferenceDataset(batch_size=batch_size, workers=cpu_workers,
        gzip=gzip)
    (x_data, hist_data, single_card_data)  = dataset.get_next(is_training)

  if args.linear:
    model = InferenceLinear(model_def["name"], config["input_shape"], x_data,
        hist_data, single_card_data, config["learning_rate"], args.hist,
        args.sink)
  else:
    model = InferenceFCNet(model_def["name"], config["input_shape"], x_data,
        hist_data, single_card_data, config["learning_rate"], args.hist,
        args.sink)

  model.dataset = dataset
  model.train_set = model_def["train"]
  model.test_set = model_def["test"]
  model.max_epochs = model_def["max_epochs"]
  models.append(model)


model_path = os.path.join(args.output_dir, args.model_name)
output_variables_path = os.path.join(model_path, VARIABLES_EXT)
builder = tf.saved_model.builder.SavedModelBuilder(model_path)
util.serialize_model_defs(models, model_path)
results_file = os.path.join(model_path, "train.out")

# experiment loop
with tf.Session() as sess:
  sess.run(tf.global_variables_initializer())
  
  builder.add_meta_graph_and_variables(sess, 
      [tf.saved_model.tag_constants.TRAINING])
  builder.save()

  # for saving variables every epoch
  saver = tf.train.Saver()
  
  for model in models:
    util.log(results_file, "*** Training {} ***\n".format(model.name))
    
    prev_test_loss = np.inf
    for epoch in range(model.max_epochs):

      ep_train_losses = []
      ep_test_losses = []
      train_losses = []
      test_losses = []

      model.dataset.refresh_train(sess, model.train_set)
      feed_dict = {model.keep_prob: 0.8, is_training: True}
      ops = [model.train_step, model.loss]
      i = 0
      while True:
        try:
          (_, trl)  = sess.run(ops, feed_dict=feed_dict)
          if np.shape(trl)[0] == batch_size and i % 3 == 0:
            ep_train_losses.append(trl)
            train_losses.append(trl)
          i += 1

          global_step = sess.run(model.global_step)
          if ((global_step % config["steps_per_log"]) == 0):
            # make an additional run over the test set
            model.dataset.refresh_test(sess, model.test_set)
            old_fd = feed_dict
            old_ops = ops
            feed_dict = {model.keep_prob: 1.0, is_training: False}
            ops = [model.loss]
            j = 0
            while True and j < config["steps_per_log"]:
              try:
                tl  = sess.run(ops, feed_dict=feed_dict)[0]
                if np.shape(tl)[0] == batch_size:
                  test_losses.append(tl)
                j += 1
                  
              except tf.errors.OutOfRangeError:
                break
            
            util.log(results_file, "Step={} ".format(global_step))
            util.log(results_file, "train_loss={:.9f} ".format(np.mean(train_losses)))
            util.log(results_file, "test_loss={:.9f} ".format(np.mean(test_losses)))
            util.log(results_file, "\n")
            train_losses = []
            test_losses = []
            
            # restore feed dict and ops
            feed_dict = old_fd
            ops = old_ops

        except tf.errors.OutOfRangeError:
          break

      model.dataset.refresh_test(sess, model.test_set)
      feed_dict = {model.keep_prob: 1.0, is_training: False}
      ops = [model.loss]
      while True:
        try:
          tl  = sess.run(ops, feed_dict=feed_dict)[0]
          if np.shape(tl)[0] == batch_size:
            ep_test_losses.append(tl)
            
        except tf.errors.OutOfRangeError:
          break

      avg_test_loss = np.mean(ep_test_losses)
      if avg_test_loss < prev_test_loss:
        prev_test_loss = avg_test_loss
        # only save when the model has improved
        saver.save(sess, output_variables_path)
      
      # Display logs per epoch step
      util.log(results_file, "Epoch={} ".format(epoch+1))
      util.log(results_file, "train_loss={:.9f} ".format(np.mean(ep_train_losses)))
      util.log(results_file, "test_loss={:.9f} ".format(np.mean(ep_test_losses)))
      util.log(results_file, "\n")
    
