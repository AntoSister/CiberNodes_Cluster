import numpy as np
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
from sat_db import SatDataBase
import time
import datetime
from threading import Thread
import subprocess
import re
import logging
import sys
import pandas as pd
import argparse

logging.basicConfig(format='%(levelname)s:%(message)s', encoding='utf-8', level=logging.INFO)
# logging.debug('This message should go to the log file')
# logging.info('So should this')

def stop_constellation():
    # kill -SIGINT $(ps aux | grep '[p]ython constellation_topology.py' | awk '{print $2}')
    ps_res = subprocess.run(['ps', 'aux'], capture_output=True, text=True)
    ps_out = ps_res.stdout
    cons_ps = [ps for ps in ps_out.split('\n') if 'python constellation_topology.py' in ps]
    pids = [re.split(' +',ps)[1] for ps in cons_ps]
    subprocess.run(['kill', '-SIGINT'] + pids)


def set_mec(db, ip, mec_on):
    servers_cores = db.get_items_in_collection(query={'ip': ip}, collection='servers')
    for s in servers_cores:
        s['mec_on'] = mec_on
        db.upsert_in_collection(s, collection='servers')

def get_env_variables_pretrain(df, timestep, ep):
    t = (ep * 19) % df.shape[0] + timestep
    servers = np.array(train_df['servers'][t][1:-1].replace("'", '').split(' '))
    states = np.array([re.split(' +',r.strip()) for r in df['inputs'][t].replace('[', '').replace(']', '').split('\n')]).astype(float)
    rewards = np.array(re.split(' +',df['rewards'][t][1:-1].strip())).astype(float)
    actions = np.array(df['actions'][timestep][1:-1].split(',')).astype(int)
    return servers, states, rewards, actions

def get_env_variables(db, step=60, w1=1.0, w2=1.0, w3=1.0, w4=1.0):
    time_now = time.time()
    all_tasks =  db.get_items_in_collection(collection='tasks', 
                                                query={'send_time': {'$gt': time_now-step}})
    # cpu_hist = self.db.get_items_in_collection(collection='servers_hist', query={'name': self.ip,
    #                                                                   'mec': True,
    #                                                                    'time': {'$gt': time_now-step}})
    cpu_hist_all = db.get_items_in_collection(collection='servers_hist',
                                                    query={'mec': True, 'time': {'$gt': time_now-step}})
    data = []
    rewards = []
    
    active_servers = list(np.unique([v['name'] for v in cpu_hist_all]))
    all_servers = list(np.unique([v['ip'] for v in db.get_items_in_collection(collection='servers') if 'st' in v['node_name']]))
    all_servers.sort()

    for ip in all_servers:

        # if ip in active_servers:
        new_active_servers = [s for s in active_servers if s != ip]
        new_all_servers = [s for s in all_servers if s != ip]
        cpu_hist = [c for c in cpu_hist_all if c['name'] == ip]
        
        cu = len(new_active_servers) / len(new_all_servers) if len(new_all_servers) != 0 else 0

        cpu = int(np.percentile([c['cpu'] for c in cpu_hist], 70)) if len(cpu_hist) > 0 else 0
        tasks = [t for t in all_tasks if ip in t['path'] and t['server']  != 'None']

        tf = 0
        to = 0
        if len(tasks) > 0:
            failed_tasks = [ t for t in tasks if t['status'] == 'Fail']
            # print('Offloaded tasks', [ [ip for ip in t['path'] if ip != t['ip']] for t in tasks])
            # print('Offloaded tasks', [ t for t in tasks if self.ip != [ip for ip in t['path'] if ip != t['server']][-1]])
            offload_tasks = [ t for t in tasks if ip != [i for i in t['path'] if i != t['ip']][-1]]
            # print('Offloaded tasks', len(offload_tasks))

            tf = len(failed_tasks) / len(tasks)
            to = len(offload_tasks) / len(tasks)
            # total_tasks += len(tasks)
            # total_failed += len(failed_tasks)
        mec_seconds = sum([15 for c in cpu_hist if c['control_loop']])
         
        reward = w1 * np.exp(-100.0*tf)
        reward += w2 * (1.0 - mec_seconds / step )
        reward += w3 * (cpu / 100.0)
        reward += w4 * (1.0 -cu)
        data.append([tf, to, cpu, cu])
        rewards.append(reward)

    # total_mec_seconds = sum([15 for c in cpu_hist_all if c['control_loop']])
    # total_pf = total_failed / total_tasks if total_tasks != 0 else 0
    
    # reward =  w1 * (1 - total_pf) + w2* (1 - total_mec_seconds / (step * len(all_servers))) if len(all_servers) > 0 and total_tasks > 0 else 0
    return np.array(all_servers), np.array(data), np.array(rewards)

def save_parameters(model, df, date):
    now_str = now.strftime("%d%m%y%H%M%S")
    model.save_weights('{}_model_weights.h5'.format(now_str))
    df.to_csv('{}_env.csv'.format(now_str))
    # logging.debug('Killing constellation')


def get_parameters():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--timesteps', default=20, type=int, help='Timesteps per episode')
    parser.add_argument('-s', '--step', default=15, type=int, help='Seconds of each control step')
    # parser.add_argument('-l', '--load_weights', default='200324123815_model_weights.h5', type=str, help='Weights to preload')
    # parser.add_argument('-l', '--load_weights', default='170424171007_model_weights.h5', type=str, help='Weights to preload')
    # parser.add_argument('-l', '--load_weights', default='180424162453_model_weights.h5', type=str, help='Weights to preload')
    parser.add_argument('-l', '--load_weights', default='220424115222_model_weights.h5', type=str, help='Weights to preload')
    
    

    
    parser.add_argument('-d', '--data_file', default=None, type=str, help='Use pre train data file')
    # parser.add_argument('-d', '--data_file', default='130324163513_env.csv', type=str, help='Use pre train data file')
    
    return parser.parse_args()



if __name__ == '__main__':
    db = SatDataBase('172.17.0.1')
    args = get_parameters()
    # Configuration parameters for the whole setup
    seed = 42
    gamma = 0.99  # Discount factor for past rewards
    max_steps_per_episode = 50
    # env = gym.make("CartPole-v1", render_mode="human")  # Create the environment
    # env.seed(seed)
    eps = np.finfo(np.float32).eps.item()  # Smallest number such that 1.0 + eps != 1.0


    num_inputs = 4
    num_sats = 2 
    num_actions = 2
    num_hidden = 128

    inputs = layers.Input(shape=(num_inputs, ))
    common = layers.Dense(num_hidden, activation="relu")(inputs)
    common2 = layers.Flatten()(common)
    action = layers.Dense(num_actions, activation="softmax")(common2)
    critic = layers.Dense(1)(common2)

    model = keras.Model(inputs=inputs, outputs=[action, critic])
    # model = keras.Model(inputs=inputs, outputs=[critic])

    optimizer = keras.optimizers.Adam(learning_rate=0.01)
    huber_loss = keras.losses.Huber()

    model.summary()
    # logging.info('Loading weights')
    model.load_weights(args.load_weights)

    action_probs_history = []
    critic_value_history = []
    rewards_history = []
    running_reward = 0
    episode_count = 0

    data_columns = ['timestep', 'servers',	'inputs',	'actions', 'rewards']
    data = []
    now = datetime.datetime.now()

    train_df = None
    real_time = True
    next_states = None
    servers = None
    if args.data_file is not None:
        real_time = False
        train_df = pd.read_csv(args.data_file)

    try:
        while(True):
            step = args.step

            if real_time:
                logging.info('Start Constellation')
                res = subprocess.Popen(['bash' , 'run_constellation.sh'], 
                                    stdout=subprocess.DEVNULL, stderr= subprocess.DEVNULL)
                time.sleep(step*10)

            episode_reward = 0
            with tf.GradientTape() as tape:
                for timestep in range(1, max_steps_per_episode):
                    logging.info('Timestep {}'.format(timestep))
                    
                    if real_time:
                        if next_states is None:
                            servers, states, rewards = get_env_variables(db, step, 15, 7, 20, 5)
                        else:
                            states = next_states
                        
                    else:
                        servers, states, rewards, env_actions = get_env_variables_pretrain(train_df, timestep-1, episode_count)
                    logging.info('Current State {} {}'.format(servers, states))

                    actions = []
                    # Predict action probabilities and estimated future rewards
                    # from environment state
                    
                    for i , (server, state) in enumerate(zip(servers, states)):
                        action_probs, critic_value = model(tf.expand_dims(state, 0))
                        critic_value_history.append(critic_value[0, 0])
                        logging.debug('actions vector {} server {}'.format(np.squeeze(action_probs), server))

                    # Sample action from action probability distribution
                        if real_time:
                            action = np.random.choice(num_actions, p=np.squeeze(action_probs))
                        else:
                            action = env_actions[i]
                        action_probs_history.append(tf.math.log(action_probs[0, action]))
                        if real_time:
                            set_mec(db, server, True if action == 1 else False)
                        actions.append(action)

                    logging.info('Actions: {}, Servers: {}'.format(actions, servers))
                    
                    
                    if real_time:
                        time.sleep(step)
                        servers, next_states, rewards = get_env_variables(db, step,  15, 7, 20, 5)
                    logging.debug('Reward {}'.format(rewards))
                    rewards_history = rewards_history + list(rewards)
                    episode_reward += sum(rewards)
                    data.append([timestep, servers, states, actions, rewards])

                # Update running reward to check condition for solving
                running_reward = 0.05 * episode_reward + (1 - 0.05) * running_reward

                # Calculate expected value from rewards
                # - At each timestep what was the total reward received after that timestep
                # - Rewards in the past are discounted by multiplying them with gamma
                # - These are the labels for our critic
                returns = []
                discounted_sum = 0
                for r in rewards_history[::-1]:
                    discounted_sum = r + gamma * discounted_sum
                    returns.insert(0, discounted_sum)

                # Normalize
                returns = np.array(returns)
                returns = (returns - np.mean(returns)) / (np.std(returns) + eps)
                returns = returns.tolist()

                # Calculating loss values to update our network
                history = zip(action_probs_history, critic_value_history, returns)
                actor_losses = []
                critic_losses = []
                for log_prob, value, ret in history:
                    # At this point in history, the critic estimated that we would get a
                    # total reward = `value` in the future. We took an action with log probability
                    # of `log_prob` and ended up recieving a total reward = `ret`.
                    # The actor must be updated so that it predicts an action that leads to
                    # high rewards (compared to critic's estimate) with high probability.
                    if log_prob <= -np.inf:
                        continue

                    diff = ret - value
                    actor_losses.append(-log_prob * diff)  # actor loss

                    # The critic must be updated so that it predicts a better estimate of
                    # the future rewards.
                    critic_losses.append(
                        huber_loss(tf.expand_dims(value, 0), tf.expand_dims(ret, 0))
                    )

                # Backpropagation
                loss_value = sum(actor_losses) + sum(critic_losses)
                grads = tape.gradient(loss_value, model.trainable_variables)
                optimizer.apply_gradients(zip(grads, model.trainable_variables))
                logging.info('Actions Probs {}'.format(np.array(action_probs_history)))
                logging.info('Critic Values {}'.format(np.array(critic_value_history)))
                logging.info('Rewards {}'.format(rewards_history))

                # Clear the loss and reward history
                action_probs_history.clear()
                critic_value_history.clear()
                rewards_history.clear()

            # Log details
            episode_count += 1
            template = "Running reward: {:.2f} at episode {}"
            logging.info(template.format(running_reward, episode_count))

            df = pd.DataFrame(data, columns=data_columns)
            save_parameters(model, df, now)
            if real_time:
                stop_constellation()
                time.sleep(step)

            if running_reward >= 10000:  # Condition to consider the task solved
                logging.info("Solved at episode {}!".format(episode_count))
                break

            
    except Exception as e:
        df = pd.DataFrame(data, columns=data_columns)
        save_parameters(model, df, datetime.datetime.now())
        if real_time:
            stop_constellation()
        print('Exception catch {}'.format(e))
        raise e
    except KeyboardInterrupt:
        df = pd.DataFrame(data, columns=data_columns)
        save_parameters(model, df, datetime.datetime.now())
        if real_time:
            stop_constellation()
        print('KeyboardInterrupt catched')
        sys.exit(0)

    df = pd.DataFrame(data, columns=data_columns)
    save_parameters(model, df, datetime.datetime.now())

    
        

    
