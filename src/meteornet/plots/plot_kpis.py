import matplotlib
import matplotlib.pyplot as plt
import pandas as pd

import matplotlib.colors as mcolors
from matplotlib.lines import Line2D


matplotlib.rcParams.update({'font.size': 18})


df = pd.read_csv('kpis.csv')
data = df.to_numpy()

var=2

# orchs_dic = {6: 'R. Learning', 
# 4: 'Fuzzy Logic',
# 0: 'Access', 
# 7: 'Bernoulli'}

orchs = ['Access', 'Bernoulli', 'Fuzzy Logic', 'R. Learning' ]
color_index = [2, 3, 0, 1]

vars = ['$A$', '$PF$', '$PC$']

plot_colors = list(mcolors.TABLEAU_COLORS)

print(data.shape)

fig, axs = plt.subplots(nrows=1,ncols=3, sharex=True)

for var in range(3):
    for alg in range(4):
        axs[var].plot(data[:, 0], data[: ,alg*3 +1 + var], marker='x', label=orchs[alg], color = plot_colors[color_index[alg]])


    axs[var].set_ylabel(vars[var])
    if var == 0:
        axs[var].set_ylim(ymin=0, ymax=1)

ath = [x /(4 * 10) for x in data[:, 0]]
axs[0].plot(data[:, 0], ath, linestyle='--',color='black', label='$A_{Th}$')

handles, labels = axs[0].get_legend_handles_labels()

    # axs[var].set_xlabel('$\\bar{\\lambda}$')


    # orchs_dic = {6: 'R. Learning', 
    # 4: 'Fuzzy Logic',
    # 0: 'Access', 
    # 7: 'Bernoulli'}


# legend_elements = [Line2D([0], [0], linestyle=None, marker='x', color=plot_colors[color_index[i]],  label='{}'.format(orchs[i]),
#                     markerfacecolor=plot_colors[color_index[i]], markersize=10) for i, alg in enumerate(orchs)]

# legend_elements.append(Line2D([0], [0], color='black',linestyle='--', label='$A_{Th}$',
#                     markerfacecolor='black', markersize=10))
fig.legend(handles=handles, loc="lower center", ncol=5, bbox_to_anchor=(0.5, -0.01))
fig.text(0.5, 0.15, '$\\overline{\\lambda}$', ha='center')
# plt.legend()
plt.show()




