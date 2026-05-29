from mininet.net import Mininet
from mininet.log import info
from mininet.util import quietRun
from os import listdir, environ
from json import dumps, loads
from re import match, search
import argparse
import subprocess

from urllib.request import build_opener, HTTPHandler, Request


def config_sFlow(net, collector, ifname, sampling, polling):
    # info("*** Enabling sFlow:\n")
    sflow = 'ovs-vsctl -- --id=@sflow create sflow agent=%s target=%s sampling=%s polling=%s --' % (
    ifname, collector, sampling, polling)
    snames = list(net['nodes'].keys()) if isinstance(net, dict) else [s.name for s in net.switches]
    for s in snames:
        sflow += ' -- set bridge %s sflow=@sflow' % s
    # info(' '.join(snames) + "\n")
    # print('sflow command: {}'.format(sflow))
    quietRun(sflow)


def construct_topology(snames, agent, links=False):
    topo = {'nodes': {}, 'links': {}}
    for sname in snames:
        topo['nodes'][sname] = {'agent': agent, 'ports': {}}
    path = '/sys/devices/virtual/net/'
    for child in listdir(path):
        parts = match('(^.+)-(.+)', child)
        if parts == None:
            continue
        if parts.group(1) in topo['nodes']:
            ifindex = open(path + child + '/ifindex').read().split('\n', 1)[0]
            topo['nodes'][parts.group(1)]['ports'][child] = {'ifindex': ifindex}

    if links:
        links_out = str(subprocess.check_output(['ip', 'link']))
        i = 0
        for s1 in snames:
            j = 0
            for s2 in snames:
                if j > i:
                    conn = search('({}-eth.@{}-eth.|{}-eth.@{}-eth.)'.format(s1, s2, s2, s1), links_out)
                    if conn == None:
                        continue
                    intfs = conn.group().split('@')
                    # intfs = s1.connectionsTo(s2)
                    for linkName in links:
                        if s1 in linkName and s2 in linkName:
                            topo['links'][linkName] = {'node1': s1,
                                                       'port1': [intf for intf in intfs if s1 in intf][0],
                                                       'node2': s2,
                                                       'port2': [intf for intf in intfs if s2 in intf][0]}
                j += 1
            i += 1
    return topo


def send_topology(net, agent, collector):
    info("*** Sending topology\n")

    topo = net if isinstance(net, dict) else None

    if topo is None:
        topo = {'nodes': {}, 'links': {}}
        for s in net.switches:
            topo['nodes'][s.name] = {'agent': agent, 'ports': {}}
        path = '/sys/devices/virtual/net/'
        for child in listdir(path):
            parts = match('(^.+)-(.+)', child)
            if parts == None: continue
            if parts.group(1) in topo['nodes']:
                ifindex = open(path + child + '/ifindex').read().split('\n', 1)[0]
                topo['nodes'][parts.group(1)]['ports'][child] = {'ifindex': ifindex}
        i = 0
        for s1 in net.switches:
            j = 0
            for s2 in net.switches:
                if j > i:
                    intfs = s1.connectionsTo(s2)
                    for intf in intfs:
                        s1ifIdx = topo['nodes'][s1.name]['ports'][intf[0].name]['ifindex']
                        s2ifIdx = topo['nodes'][s2.name]['ports'][intf[1].name]['ifindex']
                        linkName = '%s-%s' % (s1.name, s2.name)
                        topo['links'][linkName] = {'node1': s1.name, 'port1': intf[0].name, 'node2': s2.name,
                                                   'port2': intf[1].name}
                j += 1
            i += 1

    # print(topo)

    try:
        opener = build_opener(HTTPHandler)
        request = Request('http://%s:8008/topology/json' % collector, data=dumps(topo).encode('utf-8'))
        request.add_header('Content-Type', 'application/json')
        request.get_method = lambda: 'PUT'
        url = opener.open(request)
    except Exception as e:
        print(e)
    return topo


def config_switches(net, collector='127.0.0.1',
                    ifname='lo', agent='127.0.0.1',
                    sampling='20', polling='1', config=True):
    if config:
        config_sFlow(net, collector, ifname, sampling, polling)
    topo = send_topology(net, agent, collector)
    # with open('topo.json', 'w') as outfile:
    #   outfile.write(dumps(topo))
    return

def get_parameters():
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--collector', default='127.0.0.1')
    parser.add_argument('-a', '--agent', default='127.0.0.1')
    parser.add_argument('-i', '--ifname', default='lo')
    parser.add_argument('-s', '--sampling', default=1, type=int)
    parser.add_argument('-p', '--polling', default=1, type=int)
    parser.add_argument('--snames', nargs='+', default=['ue2s'], type=str)
    return parser.parse_args()


if __name__ == "__main__":
    # args = get_parameters()
    # topo_file = open(args.topo, "r")
    # topo = loads(topo_file.read())
    # topo_file.close()
    # snames = list(topo['nodes'].keys())
    # links = list(topo['links'].keys())
    # topo = construct_topology(args.snames, args.agent)

    # config_switches(topo,
    #                 args.collector,
    #                 args.ifname,
    #                 args.agent,
    #                 args.sampling,
    #                 args.polling)

    # setattr(Mininet, 'start', wrapper(Mininet.__dict__['start']))
    # setattr(Mininet, 'send_topology', wrapper(nul))

    import pandas as pd
    import matplotlib.pyplot as plt
    import numpy as np
    import matplotlib
    import matplotlib.colors as mcolors
    import re
    import pandas as pd

    plt.rcParams["font.family"] = "sans-serif"

    flow_data = pd.read_csv('data/flows/flows.csv')

    with open('data/flows/st6_ping.txt') as f:
        delay_lines = f.readlines()

    delays = []
    times = []
    for line in delay_lines:
        # data_math = re.match(r'.*seq=(\d+) .+time=(\d+) ms.*\n', line)
        data_math = re.match(r'.*seq=(\d+) .+time=(\d+\.?\d*) ms.*\n', line)

        # if line == '64 bytes from 10.0.0.6: icmp_seq=1629 ttl=64 time=81.5 ms\n':
        #     print('hola')

        if data_math:
            times.append(float(data_math[1]))
            delays.append(float(data_math[2]))

    tasks_data = pd.read_csv('data/flows/tasks.csv').to_numpy()

    fig, ax = plt.subplots()
    ax.plot(times[1:], delays[1:], linewidth=5, label='Ping delay: gs11-st6')
    ax.plot(tasks_data[:, 1]+42, tasks_data[:, 2]*1000, '*', markersize=10, color='green', label='Task computation time')
    ax.set_ylim(0, 900)
    ax.set_xlim(0, 3600)
    ax.set_xlabel('Simulation Time [Min]', size=40)
    ax.set_ylabel('Delay [ms]', size=40)

    x_ticks = np.arange(0, 3660, 60)
    x_ticks_major = [v for v in x_ticks if v % (5*60) == 0]
    x_labels_major = [int(v/60) for v in x_ticks_major]
    x_labels_minor = [int(v/60) for v in x_ticks]
    ax.set_xticks(x_ticks, minor=True)
    ax.set_xticks(x_ticks_major, x_labels_major)
    ax.tick_params(which="major", labelsize=30)

    link_changes = [360, 990, 1620, 2250, 2880, 3480]
    sat_changes = [10, 9, 8, 7, 6, 5, 4]
    link_colors = matplotlib.colormaps['autumn'].resampled(6)

    for i, s in enumerate(link_changes):
        plt.axvline(linestyle='dashed', x=s, color=matplotlib.colors.to_hex(link_colors(i)), linewidth=3,
                    label='Transition from st{} to st{}'.format(sat_changes[i], sat_changes[i]-1))

    # plt.axvline(linestyle='-', x=1980, ymin=0.09, ymax=0.28+0.09, color='black', linewidth=5, label='Base task computing time')

    plt.arrow(x=1950, y=70, dx=0, dy=250, label='Base computing time (250ms)', length_includes_head=True,
              width=6, color='black',  joinstyle='bevel', head_starts_at_zero=True, overhang=0, shape='full')

    # ax.annotate("Base Computation", xy=(1980, 250+70), xytext=(1980, 70),
                # arrowprops=dict(arrowstyle="<->",  linewidth=4))
    ax.grid()
    ax.legend(fontsize=25)
    # plt.title('Ping and Computation Delays, gn11-st6', fontsize=30)
    plt.show()

    # reg seq=(\d+) .+time=(\d+) ms
    # reg = 'seq=(\d+) .+time=(\d+) ms'
    # delay_lines =