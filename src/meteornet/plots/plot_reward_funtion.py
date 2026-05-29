import matplotlib.pyplot as plt
import matplotlib
import numpy as np





def plot_var(x=0, var=0, ax=None, xlabel='', ylabel=''):

    y = []
    for val in x:
        y.append(reward_funtion(val, var))

    
    ax.plot(x, y, color='black')
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)

    # plt.show()


def reward_funtion(x=0, var=0):
    w1, w2, w3, w4 = 15, 7, 20, 5
    w11 = 100
    if var == 0:
        reward = w1 * np.exp(-1.0* w11*x)
    elif var == 1:
        reward = w2 * (1.0 - x)
    elif var == 2:
        reward = w3 * (x)
    elif var == 3:
        reward = w4 * (1.0 -x)
    return reward

pf = [0.01*i for i in range(101)]
po = [0.01*i for i in range(101)]
pc = [0.01*i for i in range(101)]
cu = [0.01*i for i in range(101)]



matplotlib.rcParams.update({'font.size': 22})


fig, axs = plt.subplots(ncols=4, sharey=True) 


# plt.grid()
plot_var(pf, 0, axs[0], '$\mathrm{PF}_s$', '$\omega_1 \cdot e^{-\omega_{11} \mathrm{PF}_s}$')
plot_var(po, 1, axs[1],'$\mathbb{S}_s$','$\omega_2 \cdot (1 - \mathbb{S}_s)$')
plot_var(pc, 2, axs[2], '$\mathrm{PC}_s$', '$\omega_3 \cdot \mathrm{PC}_s$')
plot_var(cu, 3, axs[3], '$\mathrm{PU}_s$', '$\omega_4 \cdot (1.0 - \mathrm{PU}_s)$')

# plt.legend()
# plt.ylabel('Rewards')
# plt.xlabel('$\mathrm{PF}, \mathbb{S}, \mathrm{PC}, \mathrm{PU}$')

plt.show()