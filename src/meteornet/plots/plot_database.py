from sat_db import SatDataBase
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FormatStrFormatter
import matplotlib
import pandas as pd
import re
from datetime import datetime
import argparse
import os
import re
from functools import reduce
import warnings
import matplotlib.colors as mcolors
import random

def plot_tasks_in_time(tasks_times):
    fig, axs = plt.subplots(len(tasks_times)+1, 1)
    for n, tasks_app in enumerate(tasks_times):
        axs[n].eventplot([t['time'] for t in tasks_app])
    return fig, axs


def heatmap(data, row_labels, col_labels, ax=None,
            cbar_kw=None, cbarlabel="", **kwargs):
    if ax is None:
        ax = plt.gca()

    if cbar_kw is None:
        cbar_kw = {}

    # Plot the heatmap
    im = ax.imshow(data, **kwargs)

    # Create colorbar
    cbar = ax.figure.colorbar(im, ax=ax, **cbar_kw)
    cbar.ax.set_ylabel(cbarlabel, rotation=-90, va="bottom", fontdict={'fontsize': 30})
    cbar.ax.tick_params(which='both', labelsize=30)
    # cbar.ax.set_title(cbarlabel, fontdict={'fontsize': 20})

    # Show all ticks and label them with the respective list entries.
    ax.set_xticks(np.arange(data.shape[1]), labels=col_labels)
    ax.set_yticks(np.arange(data.shape[0]), labels=row_labels)

    # Let the horizontal axes labeling appear on top.
    ax.tick_params(top=False, bottom=True,
                   labeltop=False, labelbottom=True)

    # Rotate the tick labels and set their alignment.
    plt.setp(ax.get_xticklabels(), rotation=10, ha='center', rotation_mode='anchor')

    # Turn spines off and create white grid.
    ax.spines[:].set_visible(False)

    ax.set_xticks(np.arange(data.shape[1]+1)-.5, minor=True)
    ax.set_yticks(np.arange(data.shape[0]+1)-.5, minor=True)
    ax.grid(which="minor", color="w", linestyle='-', linewidth=3)
    ax.tick_params(which="minor", bottom=False, left=False, labelsize='large')
    ax.tick_params(which="major", axis='x', labelsize=30)
    ax.tick_params(which="major", axis='y', labelsize=30)
    plt.xlabel('Simulation Time [Min]', fontdict={'size': 30}, loc='center')

    return im, cbar


def annotate_heatmap(im, data=None, valfmt="{x:.2f}",
                     textcolors=("black", "white"),
                     threshold=None, **textkw):
    if not isinstance(data, (list, np.ndarray)):
        data = im.get_array()

    # Normalize the threshold to the images color range.
    if threshold is not None:
        threshold = im.norm(threshold)
    else:
        threshold = im.norm(data.max())/2.

    # Set default alignment to center, but allow it to be
    # overwritten by textkw.
    kw = dict(horizontalalignment="center",
              verticalalignment="center")
    kw.update(textkw)

    # Get the formatter in case a string is supplied
    if isinstance(valfmt, str):
        valfmt = matplotlib.ticker.StrMethodFormatter(valfmt)

    # Loop over the data and create a `Text` for each "pixel".
    # Change the text's color depending on the data.
    texts = []
    for i in range(data.shape[0]):
        for j in range(data.shape[1]):
            kw.update(color=textcolors[int(im.norm(data[i, j]) > threshold)])
            text = im.axes.text(j, i, valfmt(data[i, j], None), **kw)
            texts.append(text)

    return texts


def plot_net_heatmap(measures):
    flow_data = pd.DataFrame(measures)
    interface_names = flow_data['switch'].unique()
    gn_names = [n for n in interface_names if 'gn' in n]
    interface_names = list(np.sort(gn_names)) + list(np.sort([n for n in interface_names if n not in gn_names]))
    groups = pd.cut(flow_data['sim_time'], [i*60 for i in range(61)])
    flow_groups = flow_data.groupby(groups)

    # flow_data.iloc[flow_data.groupby(groups).groups[groups.values[0]], :]
    ranges_names = []
    net_usage = []
    for gr in groups.values.unique():
        gr_df = flow_data.iloc[flow_groups.groups[gr], :]
        if_values = gr_df[['switch', 'value']].groupby(by='switch').mean()
        ranges_names.append('{}'.format(int(gr.right/60)))
        usage_col = [if_values.loc[i, 'value'] if i in if_values.index else 0 for i in interface_names]
        net_usage.append(usage_col)

    net_usage = np.array(net_usage).T

    fig, ax = plt.subplots()
    im, cbar = heatmap(net_usage, interface_names, ranges_names, ax=ax,
                       cmap="YlGn", cbarlabel="Network Usage [bps]")
    # texts = annotate_heatmap(im, valfmt="{x:.0f} bps")

    fig.tight_layout()
    # plt.title('Network Measurements for Link Interfaces', fontsize=40)
    plt.show()


def plot_tasks_sources(task_items, nodes_map, times=None, delays=None):

    success_indexes = [i for i, t in enumerate(task_items) if t['status'] == 'Done']
    success_tasks = np.array(task_items)[success_indexes]

    fig, ax = plt.subplots(1, 1)
    normalized_times = np.array([t['send_time'] - success_tasks[0]['send_time'] for t in success_tasks])
    # Plot task computations times
    ax.set_title('Task Computation Times, {} Total Tasks'.format(len(success_tasks)), fontdict={'size':40})
    for ip in np.unique([t['ip'] for t in success_tasks]):
        norm_times = np.array([t['computation_time'] for t in success_tasks])
        ip_indexes = [i for i, t in enumerate(success_tasks) if t['ip'] == ip]
        ax.plot(normalized_times[ip_indexes],
                 norm_times[ip_indexes], 'x',
                     label="Tasks generated in {}: {} $\\mu = {:.3f}, \\sigma = {:.3f}$".
                format(nodes_map[ip], len(norm_times[ip_indexes]), norm_times[ip_indexes].mean(), norm_times[ip_indexes].std()))

    # for t, d in zip(times, delays):
    # ax.plot(times[0], delays[0], 'o', label='ICMP from gn11 to st2 (1750 Secons)')
    # ax.plot(times[1], delays[1], 'o', label='ICMP from gn12 to st2 (500 Seconds)')
    ax.legend(fontsize=20)
    ax.grid()
    ax.tick_params(which="major", labelsize=30)
    ax.set_ylim(0.02, 2.5)
    # ax.set_xlim(0, 2050)
    ax.set_xlabel('Emulation Time [Seconds]', size=35)
    ax.set_ylabel('Computation Time [Seconds]', size=35)
    plt.show()


def plot_tasks_servers(task_items, nodes_map):
    fig, ax = plt.subplots(1, 1)
    normalized_times = np.array([t['send_time'] - task_items[0]['send_time'] for t in task_items])

    fail_indexes = [i for i, t in enumerate(task_items) if t['status'] == 'Fail' and t['server']  != 'None' and not isinstance(t['server'], float)]
    server_fail_indexes = [i for i, t in enumerate(task_items) if t['status'] == 'Fail' and (t['server']  == 'None' or isinstance(t['server'], float) )]
    success_indexes = [i for i, t in enumerate(task_items) if t['status'] == 'Done']
    fail_tasks = np.array(task_items)[fail_indexes]
    success_tasks = np.array(task_items)[success_indexes]
    success_norm_times = np.array([t['computation_time'] for t in success_tasks])

    for ip in np.unique([t['server'] for t in success_tasks]):
        ip_indexes = [i for i, t in enumerate(success_tasks) if t['server'] == ip]
        ax.plot(normalized_times[success_indexes][ip_indexes],
                 success_norm_times[ip_indexes], 'x',
                     label='Success Tasks Processed in {}: {} $\\mu = {:.3f}, \\sigma = {:.3f}$'.
                format(nodes_map[ip], len(success_tasks[ip_indexes]), success_norm_times[ip_indexes].mean(),
                       success_norm_times[ip_indexes].std()))

    # ax.plot(normalized_times[fail_indexes],
    #          [-1 for t in fail_tasks], 'o', color='red',
    #              label='Failed Tasks: {}'.format(len(fail_tasks)))
    ax.set_xlabel('Emulation Time [Seconds]', size=30)
    ax.set_ylabel('Computation Time [Seconds]', size=30)
    ax.set_title('Tasks Computation Time\n {} Total Tasks, {} Success Tasks, \n{} Server Fail, {} Network Fail'.format(
        len(task_items), len(success_tasks), len(fail_tasks), len(np.array(task_items)[server_fail_indexes]) ), size=30)
    ax.tick_params(which="major", labelsize=30)
    ax.legend(fontsize=20)
    plt.show(block=False)

def plot_interfaces_measures(net_measures):
    fig, ax = plt.subplots(1, 1)

    values = np.array([n['value'] for n in net_measures])
    times = np.array([n['sim_time'] for n in net_measures])
    interfaces = np.unique([n['switch'] for n in net_measures])

    for intf in interfaces:
        indexes = np.array([n['switch'] for n in net_measures]) == intf
        ax.plot(times[indexes], values[indexes], label=intf)
    ax.legend()
    plt.show()


def plot_tasks_boxes(task_items):
    fig, ax = plt.subplots(1, 1)
    n_steps = 15

    success_indexes = [i for i, t in enumerate(task_items) if t['status'] == 'Done']
    success_tasks = np.array(task_items)[success_indexes]

    computation_times = np.array(
        [t['computation_time'] if t['computation_time'] != 'None' else None for t in success_tasks])
    normalized_times = np.array([t['send_time'] - task_items[0]['send_time'] for t in success_tasks])
    split = [int(len(computation_times) / n_steps) * (s + 1) for s in range(n_steps)]
    split_times = [np.mean(d) for d in np.split(normalized_times, split)[:n_steps]]
    compt_times_in_steps = np.split(computation_times, split)[:n_steps]
    compt_times_in_steps = [l[l != np.array(None)].astype(float) for l in compt_times_in_steps]

    ax.violinplot(compt_times_in_steps, positions=split_times, widths=int(np.mean(split_times)/n_steps),
                  showmeans=True, showmedians=False, showextrema=True)

    # ax.boxplot(compt_times_in_steps, positions=split_times, widths=np.max(normalized_times)/(n_steps*2), patch_artist=True,
    #            showmeans=True, showfliers=False,
    #            medianprops={"color": "white", "linewidth": 0.5},
    #            boxprops={"facecolor": "C0", "edgecolor": "white",
    #                      "linewidth": 0.5},
    #            whiskerprops={"color": "C0", "linewidth": 1.5},
    #            capprops={"color": "C0", "linewidth": 1.5})
    # ax.set_xlim(0, np.max(normalized_times))
    ax.xaxis.set_major_formatter(FormatStrFormatter('%.2f'))
    plt.show(block=False)


def plot_servers_usages(servers_hist, nodes_map):
    fig, ax = plt.subplots(1, 1)
    servers_name = np.unique([v['name'] for v in servers_hist])

    # normalized_times = np.array([t['send_time'] - success_tasks[0]['send_time'] for t in success_tasks])
    normalized_times = np.array([v['time'] - servers_hist[0]['time'] for v in servers_hist])
    for s_name in servers_name:
        name_indexes = [i for i, t in enumerate(servers_hist) if t['name'] == s_name]
        server_values = np.array(servers_hist)[name_indexes]
        ax.plot(normalized_times[name_indexes], 
                [v['cpu'] for v in server_values],'*', label=nodes_map[s_name])
    ax.legend(fontsize=20)
    ax.set_title('CPU Utilization in MEC', size=30)
    ax.set_xlabel('Emulation Time [Seconds]', size=30)
    ax.set_ylabel(' CPU Utilization [%]', size=30)
    ax.tick_params(which="major", labelsize=30)
    ax.set_ylim([0, 100])
    ax.set_xlim(xmin=0)
    plt.show(block=False)

def plot_fuzzy_variables(servers_hist, nodes_map, target_names, reference_time):
    servers_name = np.unique([v['name'] for v in servers_hist])
    # target_names = list(nodes_map.values())
    # target_names = ['st6', 'st7', 'st8', 'st9']
    # target_names = ['st9']
    fig, ax = plt.subplots(len(target_names), 1)
    if (len(target_names) == 1):
        ax = [ax]
    
    normalized_times = np.array([v['time'] - reference_time for v in servers_hist])
    for s_name in servers_name:
        if nodes_map[s_name] not in target_names:
            continue
        s_i = target_names.index(nodes_map[s_name])
        name_indexes = [i for i, t in enumerate(servers_hist) if t['name'] == s_name]
        server_values = np.array(servers_hist)[name_indexes]
        ax[s_i].set_title('{}'.format(nodes_map[s_name]), size=10)
        ax[s_i].plot(normalized_times[name_indexes], 
                [v['fuzzy_output'] for v in server_values], label='Fuzzy output')
        ax[s_i].plot(normalized_times[name_indexes], 
                [v['tasks_failed'] for v in server_values],'x', label='Norm Failed Tasks')
        ax[s_i].plot(normalized_times[name_indexes], 
                [v['tasks_offloaded'] for v in server_values],'*', label='Norm Offloaded Tasks')
        ax[s_i].plot(normalized_times[name_indexes], 
                [v['constellation_usage'] for v in server_values], '^', label='Constellation Usage', color='gray')
        ax[s_i].plot(normalized_times[name_indexes], 
                [v['mec'] for v in server_values], label='MEC ON', color='red')
        
        
        # ax[s_i].legend(fontsize=10)
        # ax2 = ax[s_i].twinx() 
        # ax2.set_ylabel('CPU') 
        ax[s_i].plot(normalized_times[name_indexes], 
                np.array([v['mean_cpu'] for v in server_values]) / 100, '*', label='Mean Cpu', color='cyan')
        ax[s_i].set_xlim([0, np.max(normalized_times)])
        ax[s_i].set_ylim([-0.1, 1.1])
        # ax[s_i].set_xlim([0, 1000])
        # ax2.set_ylim([-10, 110])
        # ax2.legend(loc='center right')
        
        
    plt.show()


def process_icmp_file(file):

    with open(file) as f:
        delay_lines = f.readlines()

    delays = []
    times = []
    for line in delay_lines:
        data_math = re.match(r'.*seq=(\d+) .+time=(\d+\.?\d*) ms.*\n', line)
        if data_math:
            times.append(float(data_math[1]))
            delays.append(float(data_math[2]))

    return np.array(times), np.array(delays) / 1000.0


def export_data(tasks, servers_hist, nodes, sys_data, conn_hist):
    now = datetime.now()
    n_sat_servers  = len([n for n in nodes if 'st' in n['name'] and n['ip'] != ''])
    dir_name = now.strftime(f"data/sims/{sys_data['n_sat']}sats_{sys_data['n_gnd']}gnd_{n_sat_servers}sts_{sys_data['n_ggs']}ggs_{sys_data['devices']}dev_{sys_data['total_seconds']}seconds_{sys_data['orchestration']}orch_%d-%m-%y_%H-%M-%S")
    tasks_df = None
    servers_df = None
    nodes_df = None
    netm_df = None
    os.mkdir(dir_name)
    if len(tasks) > 0:
        tasks_df = pd.DataFrame(tasks)
        tasks_df.to_csv('{}/tasks.gz'.format(dir_name), index=False)
    if len(servers_hist) > 0:
        servers_df = pd.DataFrame(servers_hist)
        servers_df.to_csv('{}/servers.gz'.format(dir_name), index=False)
    if len(nodes) > 0:
        nodes_df = pd.DataFrame(nodes)
        nodes_df.to_csv('{}/nodes.gz'.format(dir_name), index=False)
    if len(conn_hist) > 0:
        conn_df = pd.DataFrame(conn_hist)
        conn_df.to_csv('{}/conn_hist.gz'.format(dir_name), index=False)

    return tasks_df, servers_df, nodes_df, netm_df

def get_dataset(db, data_dir=None):
    task_items = []
    servers_hist_all = []
    nodes = []
    # net_measures = []
    if data_dir is not None:
        task_items = pd.read_csv('{}/tasks.gz'.format(data_dir)).to_dict('records')
        servers_hist_all = pd.read_csv('{}/servers.gz'.format(data_dir)).to_dict('records')
        nodes = pd.read_csv('{}/nodes.gz'.format(data_dir)).to_dict('records')
        # net_measures = pd.read_csv('{}/net_measures.gz'.format(data_dir)).to_dict('records')
    else:
        task_items = db.get_items_in_collection(collection='tasks')
        servers_hist_all = db.get_items_in_collection(collection='servers_hist', sort=('time', 1))
        nodes  = db.get_items_in_collection(collection='nodes')
        # net_measures = db.get_items_in_collection(collection='net_measures')

    # servers_hist_mec_on = db.get_items_in_collection(collection='servers_hist', query={'mec': True}, sort=('time', 1))
    servers_hist_mec_on = [t for t in servers_hist_all if t['mec']]
    # servers_hist_control = db.get_items_in_collection(collection='servers_hist', query={'control_loop': True}, sort=('time', 1))
    servers_hist_control = [t for t in servers_hist_all if t['control_loop']]
    nodes_map = {n['ip']: n['name'] for n in nodes}
    reference_time = task_items[0]['send_time'] if len(task_items) > 0 else -1

    return [task_items, servers_hist_all, nodes, servers_hist_mec_on, servers_hist_control, nodes_map, reference_time]

def plot_bar_comparison(groups_devs, groups_values, tasks_fails, cpu_means, n_sats , n_gnd, use_dev, use_geo):
    fs = 28
    x = np.arange(len(groups_devs))  # the label locations
    width = 0.35  # the width of the bars
    multiplier = 0

    # fig, ax = plt.subplots(len(groups_devs), 1, layout='constrained')
    # for attribute, measurement in cpu_means.items():

    #     markers = ['v', '^']
    #     for i, m in enumerate(measurement):
    #         m[np.isnan(m)] = 0
    #         ax[i].stem([i*15 for i in range(len(m))], m, linefmt='blue' if attribute == 'fuzzy' else 'orange', markerfmt=markers[0 if attribute == 'fuzzy' else 1], basefmt=' ',
    #                 label='{} control, $\mu$ {:.1f}, '.format(attribute, np.nanmean(m)))
    #         ax[i].set_title('{} devices'.format(groups_devs[i]))
    #         ax[i].set_ylim(0, 100)
    #         ax[i].set_ylabel('CPU Usage [%]')
    #         ax[i].legend()
            
    
    # ax[0].set_title('Mean CPU Usage \n {} devices'.format(groups_devs[0]))
    # ax[-1].set_xlabel('Emulation Time [Seconds]', size=20)

    plt.rcParams['font.size'] = fs
    fig, ax = plt.subplots(layout='constrained')
    

    for attribute, measurement in groups_values.items():
        mess = np.mean(measurement, axis=1).astype(int)
        yerr = np.std(measurement, axis=1)
        offset = width * multiplier
        rects = ax.bar(x + offset, mess, width, label='{} Orchestration'.format(attribute.capitalize()), yerr=yerr)
        ax.bar_label(rects, padding=3)
        multiplier += 1

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('A [s]', fontsize=fs)
    ax.set_xlabel('Devices per GS' if use_dev else '$G$', fontsize=fs)
    ax.set_title('')
    # ax.set_title('Total Time MEC Servers Active \n For {} Satellites, '.format(n_sats) + ('{} Ground Stations'.format(n_gnd) if use_dev else '{} Devices per GS'.format(200)), fontsize = 40)
    ax.set_xticks(x + width/2, groups_devs)
    ax.tick_params(axis='both', which='major', labelsize=fs)
    ax.legend(loc='upper left', fontsize=fs)
    ax.grid(axis='y')
    ax.set_ylim(0, 9700)
    ax.spines[['right', 'top']].set_visible(False)
    plt.show()

    plt.rcParams['font.size'] = fs
    fig, ax = plt.subplots(layout='constrained')
    multiplier = 0
    count = 0 
    for attribute, measurement in tasks_fails.items():
        offset = width * multiplier
        mess = np.mean(measurement, axis=1).astype(int)
        err = np.std(measurement, axis=1)
        y = np.array([m[1] for m in mess])
        yerr = np.array([m[1] for m in err])

        yfail = np.array([m[0] for m in mess])
        yfailerr = np.array([m[0] for m in err])

        # ax.errorbar(x +offset, y-yerr, yerr, fmt='o', linewidth=2, capsize=6, lolims=True)
        rects = ax.bar(x +offset, y-yfail, width=width, label='Sucess Tasks for {} Orchestration'.format(attribute.capitalize()), yerr=yerr)
        ax.bar_label(rects, fmt='%d', label_type='center')

        rects = ax.bar(x +offset, yfail, width=width, bottom=y-yfail, color='red', label='$\\mathrm{PF}$ [%]' if count !=0 else None, yerr=yfailerr, ecolor='purple')
        ax.bar_label(rects, fmt='%d', labels=['{:.2f}%'.format(yp) for yp in  yfail*100/y])

        count += 1

        print('y value {} yerr {}'.format(y, yerr))

        multiplier += 1

    ax.set_xticks(x + width/2, groups_devs)
    ax.tick_params(axis='both', which='major', labelsize=fs)
    # ax.set_title('Tasks VS Devices \n For {} Satellites, '.format(n_sats) + ('{} Ground Stations'.format(n_gnd) if use_dev else '{} Devices per GS'.format(200)), fontsize=30)
    ax.legend(loc='upper left', fontsize=fs, framealpha=0.3)

    ax.set_ylabel('Number of Tasks', fontsize=fs)
    ax.set_xlabel('Devices per GS' if use_dev else '$G$', fontsize=fs)
    ax.grid(axis='y')
    ax.set_ylim(0, 80000)
    ax.spines[['right', 'top']].set_visible(False)

    
    plt.show()


def parse_dataset_dir(base_dir):
    datasets_names = os.listdir(base_dir)
    all_devs = []
    all_orchs = []
    all_times = []
    all_fail = []
    all_tasks = []
    all_cpu = []
    all_gnd = []
    all_geo = []
    all_info = []

    n_gnd = 0
    n_sats = 0
    devices = 0
    wk = 1

    for name in datasets_names:
        data_dir = '{}/{}'.format(base_dir, name)
        # datasets.append(get_dataset(db, data_dir=data_dir))
        task_items, servers_hist_all, nodes, servers_hist_mec_on, servers_hist_control, nodes_map, reference_time, net_measures = get_dataset(db, data_dir=data_dir)
        devices = int(re.search("_(\\d+)dev_", name).group(1))
        n_gnd = int(re.search("_(\\d+)gnd_", name).group(1))
        orch = int(re.search("_(\\d+)orch_", name).group(1))
        n_sats = int(re.search("^(\\d+)sats_", name).group(1))
        geo_type = re.search("_\\d+orch_(.)", name).group(1)
        geo_type = geo_type if geo_type in ['u', 's', 'r'] else 'None'


        add_info = re.search('_([A-Za-z0-9]{3,7})$', name).group(1) if re.search('_([A-Za-z0-9]{3,7})$', name) is not None else 'None'

        all_devs.append(devices)
        all_orchs.append(orch)
        all_gnd.append(n_gnd)
        all_geo.append(geo_type)
        all_info.append(add_info)

        total_mec_seconds = 0
        servers_ip = np.unique([s['name'] for s in servers_hist_control if s['mec']])
        cpu_usage = []
        for ip in servers_ip:
            s_hist_control_ip = [s for s in servers_hist_control if s['name'] == ip]
            s_hist_control_ip.sort(key=lambda v:v['time'])
            mec_ip = [g['mec'] for g in s_hist_control_ip]

            total_mec_seconds +=sum([15 for m in mec_ip if m])

        # max_len = max([len(c) for c in cpu_usage])
        # cpu_usage = np.array([c if len(c) == max_len else np.concatenate((c,[None]*(max_len - len(c)))) for c in cpu_usage]).astype(float)

        with warnings.catch_warnings():
            warnings.simplefilter('ignore', category=RuntimeWarning)
            cpu_usage_mean = np.mean(cpu_usage, axis=0, where=np.logical_not(np.isnan(cpu_usage)))

        # cpu_usage_mean = [ np.any(non_nan) for non_nan in np.logical_not(np.isnan(cpu_usage))]
        all_cpu.append(cpu_usage_mean)

        # total_mec_seconds2 = sum([15 for s in servers_hist_control if s['mec']]) / wk
        # print(total_mec_seconds2)

        server_fail_tasks = [t for t in  task_items if t['status'] == 'Fail' and not isinstance(t['server'], float)]
        fail_tasks = len(server_fail_tasks) / len(task_items)
        all_fail.append(len(server_fail_tasks))
        all_tasks.append(len([t for t in task_items if not isinstance(t['server'], float)]))
        all_times.append(total_mec_seconds)


        print('For {} gnd,  {} dev, {} orchestration got: {} seconds with fail tasks: {}, name: {}'.format(n_gnd, devices, 'access' if orch==0 else 'fuzzy', total_mec_seconds, fail_tasks, name))
        # ip_mec_on = np.unique([v['name'] for v in servers_hist_mec_on])
        # target_ip = np.unique([v['name'] for v in servers_hist_control if v['mec'] or v['tasks_offloaded'] != 0])
        # plot_fuzzy_variables(servers_hist_control, nodes_map, [nodes_map[ip] for ip in target_ip if 'st' in nodes_map[ip]], reference_time)

    use_dev = len(np.unique(all_devs)) > 1
    use_geo = len(np.unique(all_geo)) > 1
    # use_geo = False
    groups_devs = np.sort(np.unique(all_devs)).astype(str) if use_dev else np.sort(np.unique(all_gnd)).astype(str)
    groups_devs = groups_devs if not use_geo else np.sort(np.unique(all_geo)).astype(str)


    groups_values = {'fuzzy' : [[] for i in range(len(groups_devs))],
                        'access' : [[] for i in range(len(groups_devs))]}

    tasks_fails = {'fuzzy' : [[] for i in range(len(groups_devs))],
                    'access' : [[] for i in range(len(groups_devs))]}
    
    cpu_means = {'fuzzy' : [[] for i in range(len(groups_devs))],
                'access' : [[] for i in range(len(groups_devs))]}
    
    infos = {'fuzzy' : [[] for i in range(len(groups_devs))],
            'access' : [[] for i in range(len(groups_devs))]}
    
    
    for i, t in enumerate(all_times):
        if use_dev:
            groups_values['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_devs[i]))].append(t)
            tasks_fails['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_devs[i]))].append((all_fail[i], all_tasks[i]))
            cpu_means['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_devs[i]))].append(all_cpu[i])
            infos['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_devs[i]))].append(all_info[i])
        elif use_geo:
            groups_values['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_geo[i]))].append(t)
            tasks_fails['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_geo[i]))].append((all_fail[i], all_tasks[i]))
            cpu_means['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_geo[i]))].append(all_cpu[i])
            infos['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_geo[i]))].append(all_info[i])
        else:
            groups_values['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_gnd[i]))].append(t)
            tasks_fails['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_gnd[i]))].append((all_fail[i], all_tasks[i]))
            cpu_means['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_gnd[i]))].append(all_cpu[i])
            infos['access' if all_orchs[i]==0 else 'fuzzy'] [list(groups_devs).index(str(all_gnd[i]))].append(all_info[i])

    return groups_devs, groups_values, tasks_fails, cpu_means, n_sats , n_gnd, use_dev, use_geo, infos

def get_parameters():
    parser = argparse.ArgumentParser()
    parser.add_argument('--export', default=False, action='store_true')
    parser.add_argument('--use_storage', default=False, action='store_true')
    parser.add_argument('--single', default=True, action='store_true')
    # parser.add_argument('--type', default='scatter', choices=['bar', 'scatter'])
    # parser.add_argument('--data_dirs', nargs='+', default=['data/sims/10sats_3gnd_14-11-23', 'data/sims/10sats_200dev_16-11-23'],
    #                      type=str, help='List of datasets dirs')
    parser.add_argument('--data_dirs', nargs='+', default=['data/sims/rl','data/sims/access'],
                        type=str, help='List of datasets dirs')
    # parser.add_argument('--data_dirs', nargs='+', default=['data/sims/sims_fuzzy_metaparameters'],
    #                     type=str, help='List of datasets dirs')
    return parser.parse_args()


if __name__ == '__main__':
    db = SatDataBase('172.17.0.1')

    args  = get_parameters()
    datasets = []

    if args.single or args.export:
        data_dir = None

        if args.use_storage:
            datasets = os.listdir('data/sims')
            data_dir = 'data/sims/06-05-24/{}'.format('10sats_3gnd_200dev_1000seconds_7orch_06-05-24_19-04-21')
            # data_dir = 'data/sims/06-05-24/{}'.format('10sats_3gnd_200dev_1000seconds_6orch_06-05-24_20-00-59')
            print('Plotting dataset {}'.format(data_dir))

        task_items, servers_hist_all, nodes, servers_hist_mec_on, servers_hist_control, nodes_map, reference_time= get_dataset(db, data_dir=data_dir)

        if args.export and not args.use_storage:
            sys_data = db.get_item_in_collection(collection='sim_status')
            conn_hist = db.get_items_in_collection(collection='connections_hist')
            export_data(task_items, servers_hist_all, nodes, sys_data, conn_hist)
        else:     
            # plot_tasks_sources(task_items, nodes_map=nodes_map)
            plot_tasks_servers(task_items, nodes_map= nodes_map)

            # net_measures = db.get_items_in_collection(collection='net_measures', sort=('sim_time', 1))
            # plot_interfaces_measures(net_measures)

            # plot_net_heatmap(net_measures)

            # plot_tasks_boxes(task_items)

            plot_servers_usages(servers_hist_mec_on, nodes_map)
            ip_mec_on = np.unique([v['name'] for v in servers_hist_mec_on])
            target_ip = np.unique([v['name'] for v in servers_hist_control if v['mec'] or v['tasks_offloaded'] != 0])
            plot_fuzzy_variables(servers_hist_control, nodes_map, [nodes_map[ip] for ip in target_ip if 'st' in nodes_map[ip]], reference_time)
    else:
        print('Multiple sims')
        # base_dir = 'data/sims/10sats_3gnd_14-11-23' 
        # base_dir = 'data/sims/10sats_200dev_16-11-23'
        # base_dir = 'data/sims/10sats_200dev_g_16-11-23'

        for base_dir in args.data_dirs:
            groups_devs, groups_values, tasks_fails, cpu_means, n_sats , n_gnd, use_dev, use_geo, info = parse_dataset_dir(base_dir)
            plot_bar_comparison(groups_devs, groups_values, tasks_fails, cpu_means, n_sats , n_gnd, use_dev, use_geo)







        
        



