import tensorflow as tf
import os
import util
from layers import conv_layer, batch_norm_layer, resnet_block


class Model:
  def __init__(self):
    self.get_graphname = "base-model"

  def make_graph(self):
    raise NotImplementedError()


class MLP(Model):
  def __init__(self, num_layers=0, hidden_sizes=[], apply_dropout=[], rect=tf.nn.relu,
      name="mlp", lr=1e-4, max_grad_norm=1.0):
    self.name = name
    self.num_layers = num_layers
    self.hidden_sizes = hidden_sizes
    self.apply_dropout = apply_dropout
    self.rect = rect
    self.lr = lr
    self.max_grad_norm = max_grad_norm
    self.batches = 0

  def get_prefix(self):
    return "{}/".format(self.name)


  def make_graph(self, input_shape, output_shape, keep_prob, legal_actions,
      loss_func, rnn=False, input_x=None):
    self.output_shape = output_shape
    self.legal_actions = legal_actions
    self.rnn = rnn
    
    with tf.name_scope(self.name):
      if input_x is None:
        self.x = tf.placeholder("float", [None, input_shape], name="x")
      else:
        self.x = tf.placeholder_with_default(input_x, [None, input_shape], name="x")
      
      self.x = tf.cond(tf.shape(self.x)[0] < 10, lambda: tf.pad(self.x, [[0,9],[0,0]]), lambda: tf.identity(self.x))
      self.used = tf.reduce_max(tf.abs(self.x), 1)
      layer = self.x
      prev_size = input_shape

      for i in range(0, self.num_layers):
        layer = tf.layers.dense(layer, self.hidden_sizes[i],
            activation=self.rect,
            kernel_initializer=tf.contrib.layers.xavier_initializer(),
            name="{}_hidden_{}".format(self.name, i)) 

      prev_size = self.hidden_sizes[-1]
      if rnn:
        # try an rnn layer for temporal dependencies
        lstm_cell = tf.nn.rnn_cell.BasicLSTMCell(128, state_is_tuple=True)
        #lstm_cell = tf.nn.rnn_cell.MultiRNNCell([single_cell]*2, state_is_tuple=True)
        self.lstm_state_size = prev_size
        self.lstm_state_size_c = lstm_cell.state_size.c
        self.lstm_state_size_h = lstm_cell.state_size.c
        
        lstm_c_init = tf.zeros([1, self.lstm_state_size_c], name="lstm_state_init_c") 
        lstm_h_init = tf.zeros([1, self.lstm_state_size_h], name="lstm_state_init_h") 
        self.lstm_state_c_in = tf.placeholder("float", [None, self.lstm_state_size_c], name="lstm_state_in_c")
        self.lstm_state_h_in = tf.placeholder("float", [None, self.lstm_state_size_h], name='lstm_state_in_h')
        state_in = tf.nn.rnn_cell.LSTMStateTuple(self.lstm_state_c_in, self.lstm_state_h_in)
        
        rnn_in = tf.reshape(layer, [-1, 10, prev_size])
        ep_len = util.episode_length(rnn_in)
        lstm_outputs, lstm_state = tf.nn.dynamic_rnn(
            lstm_cell, rnn_in, initial_state=state_in, sequence_length=ep_len,
            scope=self.name)
        lstm_c, lstm_h = lstm_state
        self.state_out_c = tf.identity(lstm_c[:1, :], name="lstm_state_out_c")
        self.state_out_h = tf.identity(lstm_h[:1, :], name="lstm_state_out_h")
        
        layer = tf.reshape(lstm_outputs, [-1, prev_size])

      self.logits =tf.layers.dense(layer, output_shape, 
          kernel_initializer=tf.contrib.layers.xavier_initializer(),
          name="{}_logits".format(self.name))
      self.softmax_policy = tf.nn.softmax(self.logits, name="softmax_policy")
      self.legal_policy = tf.multiply(self.softmax_policy, self.legal_actions, name="legalize")
      # legal_policy will be 0 for padded states
      # numerical trick to avoid distributions with norm < 1
      self.legal_policy = tf.where(self.legal_actions > 0, self.legal_policy + 1e-8, tf.zeros_like(self.legal_policy))
      self.normalized_legal_policy = tf.divide(self.legal_policy, 
          tf.clip_by_value(tf.reduce_sum(self.legal_policy, axis=1, keepdims=True), 1e-10, 1.0), 
          name="normalized_leg_policy")

      self.policy = tf.where(self.used > 0, self.normalized_legal_policy, tf.zeros_like(self.normalized_legal_policy), name="policy")
      self.behaviour = tf.placeholder("float", [None, output_shape])

      layer = tf.layers.dense(layer, output_shape, name="{}_action_value_out".format(self.name))
      self.action_value = tf.where(self.used > 0, layer, tf.zeros_like(layer),name="action_value")
      min_action_value = tf.reduce_min(self.action_value, axis=1, keepdims=True)
      action_value_shifted = self.action_value - min_action_value + 1e-8
      action_value_legal = self.legal_actions * action_value_shifted
      argmax_action_value = tf.reduce_max(action_value_legal, axis=1, keepdims=True) + min_action_value - 1e-8
      self.argmax_action_value = tf.identity(argmax_action_value, name="argmax_action_value")
      self.value = tf.reduce_sum(self.action_value * self.policy, axis=1, keepdims=True, name="value")

      self.action = tf.placeholder(tf.float32, shape=[None, 32],
        name="action")
      self.reward = tf.placeholder(tf.float32, shape=[None, 1],
          name="reward")

      self.opt = tf.train.AdamOptimizer(self.lr)

      self.summaries_list = []
      ep_len = util.episode_length(util.batch_to_episode(self.x))
      self.avg_reward = tf.reduce_mean(util.episode_mean(self.reward, ep_len),
          name="avg_reward")
      self.avg_return = tf.reduce_mean(util.episode_sum(self.reward),
          name="avg_return")
      self.avg_val = tf.reduce_mean(util.episode_mean(self.value, ep_len), 
          name="avg_val")
      self = loss_func(self)

      self.trainable_vars = tf.get_collection(tf.GraphKeys.TRAINABLE_VARIABLES)
      self.gradients, self.variables = zip(*self.opt.compute_gradients(self.loss, self.trainable_vars))
      self.summaries_list.append(tf.summary.scalar("gradient_norm", 
        tf.global_norm(self.gradients)))
      self.gradients, _ = tf.clip_by_global_norm(self.gradients, self.max_grad_norm)

           # send key metrics to tensorboard
      self.summaries_list.append(
         tf.summary.scalar("average_value", self.avg_val))
      self.summaries_list.append(
          tf.summary.scalar("average_reward", self.avg_reward))
      self.summaries_list.append(
          tf.summary.scalar("average_return", self.avg_return))

      self.train_step = self.opt.apply_gradients(zip(self.gradients, self.variables), name="train_step")
      self.avg_rewards = []
      self.summary = tf.summary.merge(self.summaries_list, name="summaries")
      self.saver = tf.train.Saver(tf.get_collection(tf.GraphKeys.GLOBAL_VARIABLES))


  '''
  Tensors that will be accessible if the model is reloaded
  '''
  def tensor_map_to_json(self):
    tensors = { 
        "name": self.name,
        "x": self.x.name,
        "keep_prob": self.keep_prob.name,
        "legal_actions": self.legal_actions.name,
        "argmax_action_value": self.argmax_action_value.name,
        "action": self.action.name,
        "reward": self.reward.name,
        "terminal": self.terminal.name,
        "next_q": self.next_q.name,
        "avg_reward": self.avg_reward.name,
        "train_step": self.train_step.name,
        "summary": self.summary.name,
        "batches": self.batches,
        "contexts": self.contexts,
        "rnn": self.rnn
    }

    return tensors


  def from_json(self, inputs, graph):
    self.name = inputs["name"]
    self.x = graph.get_tensor_by_name(inputs["x"])
    self.keep_prob = graph.get_tensor_by_name(inputs["keep_prob"])
    self.legal_actions = graph.get_tensor_by_name(inputs["legal_actions"])
    self.argmax_action_value = graph.get_tensor_by_name(inputs["argmax_action_value"])
    self.action = graph.get_tensor_by_name(inputs["action"])
    self.reward = graph.get_tensor_by_name(inputs["reward"])
    self.terminal = graph.get_tensor_by_name(inputs["terminal"])
    self.next_q = graph.get_tensor_by_name(inputs["next_q"])
    self.avg_reward = graph.get_tensor_by_name(inputs["avg_reward"])
    self.train_step = graph.get_operation_by_name(inputs["train_step"])
    self.summary = graph.get_tensor_by_name(inputs["summary"])
    self.batches = inputs["batches"]
    self.contexts = inputs["contexts"]
    self.rnn = inputs["rnn"]


  def add_summary_writer(self, summaries_output_path, graph):
    self.writer = tf.summary.FileWriter(os.path.join(summaries_output_path,
            self.name), graph)



class ConvNet(Model):
  def __init__(self, name, input_shape, x_data, keep_prob, y, lr, 
      weight_decay=1e-4, supervised=True):
    self.name = name
    self.supervised = supervised
    self.training = tf.placeholder(tf.bool, name="is_training")
    self.global_step = tf.Variable(0, trainable=False)
    self.learning_rate = tf.train.exponential_decay(lr, self.global_step, 1000000,
        0.96, staircase=True)

    self.legal_actions = tf.placeholder(tf.float32, shape=[None, 32], name="legal_actions")
    self.terminal = tf.placeholder(tf.float32, shape=[None, 1], name="terminal")
    
    with tf.name_scope(self.name):
      self.x = tf.placeholder_with_default(x_data, [None, input_shape], name="x")
      reshaped_x = tf.reshape(self.x, [-1, 32, 30, input_shape // (32 * 30)])
      
      net = batch_norm_layer(reshaped_x, self.training)
      net = tf.nn.elu(net)
      net = conv_layer(net, 64, [7, 7], stride=[2, 3])
      for _ in range(4):
        net = resnet_block(net, 64, [3, 3], tf.nn.elu, self.training)

      net = batch_norm_layer(net, self.training)
      net = tf.nn.elu(net)
      net = conv_layer(net, 128, [3, 3])
      for _ in range(6):
        net = resnet_block(net, 128, [3, 3], tf.nn.elu, self.training)

      net = batch_norm_layer(net, self.training)
      net = tf.nn.elu(net)
      net = conv_layer(net, 256, [3,3], stride=[2, 1])
      for _ in range(8):
        net = resnet_block(net, 256, [3, 3], tf.nn.elu, self.training)
      
      net = batch_norm_layer(net, self.training)
      net = tf.nn.elu(net)
      net = conv_layer(net, 512, [3, 3])
      for _ in range(4):
        net = resnet_block(net, 512, [3, 3], tf.nn.elu, self.training)

      net = tf.layers.average_pooling2d(inputs=net, pool_size=[2, 2], strides=[2, 2])
      flat = tf.reshape(net, [-1, 4*5*512])

      self.logits = tf.layers.dense(inputs=flat, units=32)
      self.policy = tf.nn.softmax(self.logits, name="policy")

      
      # for DQN
      self.action = tf.placeholder(tf.float32, shape=[None, 32],
        name="action")
      self.reward = tf.placeholder(tf.float32, shape=[None, 1],
          name="reward")
      self.action_value = tf.identity(self.logits, name="action_value")
      min_action_value = tf.reduce_min(self.action_value, axis=1, keepdims=True)
      action_value_shifted = self.action_value - min_action_value + 1e-8
      action_value_legal = self.legal_actions * action_value_shifted
      argmax_action_value = tf.reduce_max(action_value_legal, axis=1, keepdims=True) + min_action_value - 1e-8
      self.argmax_action_value = tf.identity(argmax_action_value, name="argmax_action_value")

      self.summaries_list = []
      
      def exclude_batch_norm(name):
        return 'batch_normalization' not in name
      
      if supervised:
        self.l2_loss = tf.add_n([tf.nn.l2_loss(tf.cast(v, tf.float32)) for v in tf.trainable_variables() if exclude_batch_norm(v.name)])

        self.cross_entropy = tf.nn.softmax_cross_entropy_with_logits_v2(logits=self.logits,
            labels=y)
        self.loss = self.cross_entropy + weight_decay * self.l2_loss
      else:
        q_estimate = tf.reduce_sum(self.action_value * self.action, axis=1,
            keepdims=True)
        self.next_q = tf.placeholder(tf.float32, shape=[None, 1],
            name="next_q")
        reward_plus = model.reward + discount * model.next_q * (1 - model.terminal)
        td_error = q_estimate - tf.stop_gradient(reward_plus)
        self.loss = tf.reduce_mean(util.episode_mean(tf.square(td_error), model.ep_len)) / 2
        self.summaries_list.append(tf.summary.scalar("loss", model.loss))
        
        # metrics
        self.avg_reward = tf.reduce_mean(self.reward, name="avg_reward")
        self.avg_return = tf.reduce_mean(self.reward, name="avg_return")
        self.avg_val = tf.reduce_mean(self.argmax_action_value, name="avg_argmax_val")
        self.summaries_list.append(
           tf.summary.scalar("average_argmax_value", self.avg_val))
        self.summaries_list.append(
            tf.summary.scalar("average_reward", self.avg_reward))
        self.summaries_list.append(
            tf.summary.scalar("average_return", self.avg_return))
        self.avg_rewards = []
        self.summary = tf.summary.merge(self.summaries_list, name="summaries")
      
      
      self.optimizer = tf.train.AdamOptimizer(learning_rate=self.learning_rate)
      update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
      with tf.control_dependencies(update_ops):
        self.train_step = self.optimizer.minimize(self.loss,
            global_step=self.global_step)
      
  



  def tensor_map_to_json(self):
    if not self.supervised:
      tensors = { 
          "name": self.name,
          "x": self.x.name,
          "training": self.training.name,
          "legal_actions": self.legal_actions.name,
          "argmax_action_value": self.argmax_action_value.name,
          "action": self.action.name,
          "reward": self.reward.name,
          "terminal": self.terminal.name,
          "next_q": self.next_q.name,
          "avg_reward": self.avg_reward.name,
          "train_step": self.train_step.name,
          "summary": self.summary.name,
          "batches": self.batches,
      }
    else:
      tensors = {
          "name": self.name,
          "x": self.x.name,
          "training": self.training.name,
          "legal_actions": self.legal_actions.name,
          "train_step": self.train_step.name,
          "policy": self.policy.name
      }

    return tensors


  def from_json(self, inputs, graph):
    if not supervised:
      self.name = inputs["name"]
      self.x = graph.get_tensor_by_name(inputs["x"])
      self.training = graph.get_tensor_by_name(inputs["is_training"])
      self.legal_actions = graph.get_tensor_by_name(inputs["legal_actions"])
      self.argmax_action_value = graph.get_tensor_by_name(inputs["argmax_action_value"])
      self.action = graph.get_tensor_by_name(inputs["action"])
      self.reward = graph.get_tensor_by_name(inputs["reward"])
      self.terminal = graph.get_tensor_by_name(inputs["terminal"])
      self.next_q = graph.get_tensor_by_name(inputs["next_q"])
      self.avg_reward = graph.get_tensor_by_name(inputs["avg_reward"])
      self.train_step = graph.get_operation_by_name(inputs["train_step"])
      self.summary = graph.get_tensor_by_name(inputs["summary"])
      self.batches = inputs["batches"]
    else:
      self.name = inputs["name"]
      self.x = graph.get_tensor_by_name(inputs["x"])
      self.training = graph.get_tensor_by_name(inputs["is_training"])
      self.legal_actions = graph.get_tensor_by_name(inputs["legal_actions"])
      self.train_step = graph.get_operation_by_name(inputs["train_step"])
      self.policy = graph,get_tensor_by_name(inputs["policy"])


  def add_summary_writer(self, summaries_output_path, graph):
    self.writer = tf.summary.FileWriter(os.path.join(summaries_output_path,
            self.name), graph)



class InferenceLinear(Model):
  '''
  Baseline to test if we're doing any representation learning
  '''
  def __init__(self, name, input_shape, x_data, hist_data, single_cards, lr, 
      use_hist=False, use_sinkhorn=False):
    self.name = name
    with tf.name_scope(self.name):
      self.keep_prob = tf.placeholder_with_default(1.0, [], "keep_prob")
      self.global_step = tf.Variable(0, trainable=False)
      self.learning_rate = tf.train.exponential_decay(lr, self.global_step,
          10e7, 0.99, staircase=True)
      
      self.x = tf.placeholder_with_default(x_data, [None, input_shape],
          name="x")
      self.y = tf.placeholder_with_default(single_cards, [None, 32*4], name="y")

      if use_hist and hist_data is not None:
        self.hist = tf.placeholder_with_default(hist_data, [None, 960], name="hist")
        # concatenate with normal features
        flat = tf.concat([self.x, self.hist], 1)

      else:
        flat = self.x
        self.hist = tf.identity(self.x)


      # single cards
      self.single_cards = tf.reshape(self.y, [-1, 32, 4])
      self.single_card_logits = tf.reshape(tf.layers.dense(inputs=flat, units=32*4), [-1, 32, 4])

      if use_sinkhorn:
        sink = util.sinkhorn(tf.nn.softmax(self.single_card_logits), 10)
        logits = tf.log(tf.clip_by_value(sink, 1e-8, 1.))
      else:
        logits = self.single_card_logits
      
      self.single_card_assn = tf.nn.softmax(logits,
          name="single_card_assn")
      self.log_single_card_assn = tf.log(
          tf.clip_by_value(self.single_card_assn, 1e-8, 1.0), 
          name="log_single_card_assn")

      self.single_card_ce = tf.nn.softmax_cross_entropy_with_logits_v2(
          logits=logits, labels=self.single_cards,
          name="single_card_ce")
      self.single_card_loss = tf.reduce_mean(self.single_card_ce, axis=-1)

      self.loss = tf.identity(self.single_card_loss, name="loss")
      
      self.optimizer = tf.train.AdamOptimizer(learning_rate=self.learning_rate)
      self.train_step = self.optimizer.minimize(self.loss, global_step=self.global_step, name="train_step")
      
      
 

  def tensor_map_to_json(self):
    tensors = {
        "name": self.name,
        "x": self.x.name,
        "single_cards": self.single_cards.name,
        "train_step": self.train_step.name,
        "hist": self.hist.name,
        "single_card_assn": self.single_card_assn.name,
        "single_card_ce": self.single_card_ce.name,
        "log_single_card_assn": self.log_single_card_assn.name
    }

    return tensors


  def from_json(self, inputs, graph):
    self.name = inputs["name"]
    self.x = graph.get_tensor_by_name(inputs["x"])
    self.single_cards = graph.get_tensor_by_name(inputs["single_cards"])
    self.hist = graph.get_tensor_by_name(inputs["hist"])
    self.single_card_assn = graph.get_tensor_by_name(inputs["single_card_assn"])
    self.single_card_ce = graph.get_tensor_by_name(inputs["single_card_ce"])
    self.log_single_card_assn = graph.get_tensor_by_name(inputs["log_single_card_assn"])
    self.train_step = graph.get_operation_by_name(inputs["train_step"])


  def add_summary_writer(self, summaries_output_path, graph):
    self.writer = tf.summary.FileWriter(os.path.join(summaries_output_path,
            self.name), graph)



class InferenceFCNet(Model):
  def __init__(self, name, input_shape, x_data, hist_data, single_cards, lr,
      use_hist=False, use_sinkhorn=False):
    self.name = name

    with tf.name_scope(self.name):
      self.keep_prob = tf.placeholder_with_default(1.0, [], "keep_prob")
      self.global_step = tf.Variable(0, trainable=False)
      self.learning_rate = tf.train.exponential_decay(lr, self.global_step,
          10e7, 0.99, staircase=True)
      
      self.x = tf.placeholder_with_default(x_data, [None, input_shape], name="x")
      self.y = tf.placeholder_with_default(single_cards,
          [None, 32*4], name="y")

      if use_hist and hist_data is not None:
        self.hist = tf.placeholder_with_default(hist_data, [None, 960], name="hist")

        # reduce dimensions of history
        hist = tf.layers.dense(self.hist, units=1024, activation=tf.nn.elu)
        hist = tf.nn.dropout(hist, self.keep_prob)
        hist = tf.layers.dense(hist, units=1024, activation=tf.nn.elu)
        hist = tf.nn.dropout(hist, self.keep_prob)
        hist = tf.layers.dense(hist, units=256, activation=tf.nn.elu)
        hist = tf.layers.dense(hist, units=128, activation=tf.nn.elu)
        hist = tf.layers.dense(hist, units=64, activation=tf.nn.elu)
        # concatenate with normal features
        flat = tf.concat([self.x, hist], 1)

      else:
        flat = self.x
        self.hist = tf.identity(self.x)


      flat = tf.layers.dense(flat, units=2048, activation=tf.nn.elu)
      flat = tf.layers.dense(flat, units=1024, activation=tf.nn.elu)
      flat = tf.layers.dense(flat, units=1024, activation=tf.nn.elu)
      flat = tf.nn.dropout(flat, self.keep_prob)
      flat = tf.layers.dense(flat, units=256, activation=tf.nn.elu)
      flat = tf.nn.dropout(flat, self.keep_prob)
      flat = tf.layers.dense(flat, units=256, activation=tf.nn.elu)
      flat = tf.nn.dropout(flat, self.keep_prob)
      flat = tf.layers.dense(flat, units=256, activation=tf.nn.elu)

      # several outputs here, one for each predicted variable

      # single cards
      self.single_cards = tf.reshape(self.y, [-1, 32, 4])
      self.single_card_logits = tf.reshape(tf.layers.dense(inputs=flat, units=32*4), [-1, 32, 4])

      if use_sinkhorn:
        sink = util.sinkhorn(tf.nn.softmax(self.single_card_logits), 10)
        logits = tf.log(tf.clip_by_value(sink, 1e-8, 1.))
      else:
        logits = self.single_card_logits
      
      self.single_card_assn = tf.nn.softmax(logits, name="single_card_assn")
      self.log_single_card_assn = tf.log(
          tf.clip_by_value(self.single_card_assn, 1e-8, 1.0), 
          name="log_single_card_assn")

      self.single_card_ce = tf.nn.softmax_cross_entropy_with_logits_v2(
          logits=logits, labels=self.single_cards,
          name="single_card_ce")
      self.single_card_loss = tf.reduce_mean(self.single_card_ce, axis=-1)

      self.loss = tf.identity(self.single_card_loss, name="loss")
      
      self.optimizer = tf.train.AdamOptimizer(learning_rate=self.learning_rate)
      self.train_step = self.optimizer.minimize(self.loss, global_step=self.global_step, name="train_step")
      


  def tensor_map_to_json(self):
    tensors = {
        "name": self.name,
        "x": self.x.name,
        "y": self.y.name,
        "train_step": self.train_step.name,
        "hist": self.hist.name,
        "single_card_assn": self.single_card_assn.name,
        "single_card_ce": self.single_card_ce.name,
        "log_single_card_assn": self.log_single_card_assn.name
    }

    return tensors


  def from_json(self, inputs, graph):
    self.name = inputs["name"]
    self.x = graph.get_tensor_by_name(inputs["x"])
    self.y = graph.get_tensor_by_name(inputs["y"])
    self.hist = graph.get_tensor_by_name(inputs["hist"])
    self.single_card_assn = graph.get_tensor_by_name(inputs["single_card_assn"])
    self.single_card_ce = graph.get_tensor_by_name(inputs["single_card_ce"])
    self.log_single_card_assn = graph.get_tensor_by_name(inputs["log_single_card_assn"])
    self.train_step = graph.get_operation_by_name(inputs["train_step"])


  def add_summary_writer(self, summaries_output_path, graph):
    self.writer = tf.summary.FileWriter(os.path.join(summaries_output_path,
            self.name), graph)
