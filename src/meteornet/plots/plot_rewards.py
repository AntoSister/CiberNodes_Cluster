import matplotlib.pyplot as plt
import matplotlib
import pandas as pd
import numpy as np
import re


def get_env_variables_pretrain(df, timestep, ep):
    t = (ep * 19) % df.shape[0] + timestep
    servers = np.array(df['servers'][t][1:-1].replace("'", '').split(' '))
    states = np.array([re.split(' +',r.strip()) for r in df['inputs'][t].replace('[', '').replace(']', '').split('\n')]).astype(float)
    rewards = np.array(re.split(' +',df['rewards'][t][1:-1].strip())).astype(float)
    actions = np.array(df['actions'][timestep][1:-1].split(',')).astype(int)
    return servers, states, rewards, actions

data_file = 'rl_data/140324163621_env.csv'
#data_file = 'rl_data/200324123901_env.csv'
df = pd.read_csv(data_file)

total_episodes = int(len(df)/19)
ep_rewards = []

print('total episodes', total_episodes)
for ep in range(total_episodes):
    episode_reward = 0
    for t in range(19):
        servers, states, rewards, actions = get_env_variables_pretrain(df, t, ep)
        episode_reward += rewards.sum()
    ep_rewards.append(episode_reward)

matplotlib.rcParams.update({'font.size': 22})

# plt.plot(ep_rewards[:90])
episodes = np.array( [e+1 for e in range(total_episodes)])
p_fit = np.polyfit(episodes, ep_rewards, 2)

p = np.poly1d(p_fit)

plt.scatter(episodes, ep_rewards, marker='.', s=50)
plt.plot(episodes, p(episodes), color='red')
plt.xlabel('Episodes')
plt.ylabel('$r_e$')
plt.grid()
plt.show()



