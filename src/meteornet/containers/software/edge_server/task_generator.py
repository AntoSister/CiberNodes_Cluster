import numpy as np

# from scipy.stats import poisson
from numpy import random
from scipy.stats import poisson, expon
from enum import Enum
import json
import requests
import time
import argparse
from graph import GraphNetwork
from threading import Thread
from requests.exceptions import ConnectionError


class App(Enum):
    AUGMENTED_REALITY = 1
    E_HEALTH = 2
    HEAVY_COMP_APP = 3

    """
    Get the task poisson rate (lambda) in tasks per minute.
    """
    def get_task_rate(self):
        task_rates = [3, 2, 3]
        task_rates_dic = {a: v for a, v in zip(App, task_rates)}
        return task_rates_dic[self]

    """
    Get app container size in KiloBytes (RAM usage in server).
    """
    def get_container_size(self):
        container_sizes = [25000, 13000, 9000]
        container_sizes_dic = {a: v for a, v in zip(App, container_sizes)}
        return container_sizes_dic[self]

    """
    Get app task length in million of instructions (MI)
    """
    def get_len(self):
        instructions_len = [60000, 200000, 400000]
        instructions_len_dic = {a: v for a, v in zip(App, instructions_len)}
        return instructions_len_dic[self]

    """
    Get the results size of the task in kilobytes
    """
    def get_results_size(self):
        results_sizes = [100, 10000, 50]
        results_sizes_dic = {a: v for a, v in zip(App, results_sizes)}
        return results_sizes_dic[self]

    """
    Get the request size of the task in kilobytes
    """
    def get_request_size(self):
        results_sizes = [1500, 10000, 50]
        results_sizes_dic = {a: v for a, v in zip(App, results_sizes)}
        return results_sizes_dic[self]


    def get_delay_request(self):
        delays_reqs = [2.5, 2.5, 180.0]
        results_delays_dic = {a: v for a, v in zip(App, delays_reqs)}
        return results_delays_dic[self]


class Task:
    dev_ids = {}
    id = None
    onos_url = "http://localhost:8181"
    onos_auth = ("onos", "rocks")

    def __init__(self, time, app, dev='', id=None, ip='localhost'):
        self.dev = dev
        if id is None:
            self.get_id()
        else:
            self.id = id
        self.time = time
        self.app = app
        self.container_size = app.get_container_size()
        self.length = app.get_len()
        self.send_time = None
        self.computation_time = None
        self.ip = ip
        self._id = '{}_{}_{}'.format(self.ip, self.dev, self.id)
        self.status = 'Created'
        self.server = 'None'
        self.path = [self.ip]
        self.npath = 'None'
        self.serialize_params = ['_id', 'dev', 'id', 'ip', 'time', 'app', 'container_size', 'length',
                                 'send_time', 'computation_time', 'status', 'server', 'path', 'npath']

    def get_id(self):
        if self.dev not in Task.dev_ids:
            Task.dev_ids.update({self.dev: 0})
            self.id = 0
        else:
            Task.dev_ids[self.dev] += 1
            self.id = Task.dev_ids[self.dev]

    def __repr__(self):
        return 'T(id={}, dev={}, time={},app={})'.format(self.id, self.dev, self.time, self.app)

    def __iter__(self):
        for key in self.serialize_params:
            is_native = isinstance(self.__dict__[key], int) or isinstance(self.__dict__[key], float) or isinstance(self.__dict__[key], list)
            if is_native:
                yield key, self.__dict__[key]
            else:
                yield key, str(self.__dict__[key])

    def to_json(self):
        return json.dumps(dict(self))

    @staticmethod
    def from_json(json_task, creator='None'):
        task = Task(time=json_task['time'], app=App[json_task['app'][4:]], dev=json_task['dev'], id=json_task['id'],
                    ip=json_task['ip'])
        task.container_size = json_task['container_size']
        task.length = json_task['length']
        task.send_time = json_task['send_time']
        task.computation_time = json_task['computation_time']
        task.status = json_task['status']
        task.server = json_task['server']
        task.ip = json_task['ip']
        path = json_task['path']
       
        path.append(creator)
        # print('Path is {} for task {}'.format(path, task._id))
        task.path = path
        task.npath = json_task['npath']
        return task

    def send_task(self, server='localhost', port=5000, set_time=True, sleep=0, db=None):
        if sleep != 0:
            time.sleep(sleep)
        if set_time:
            self.send_time = time.time()
        self.status = 'Send'
        self.server = server

        try:
            if server is None:
                raise ConnectionError
            
            task_json = self.to_json()
            # print('Sending task {}'.format(task_json))
            r = requests.post('http://{}:{}/process-task'.format(server, port), json=task_json, timeout=2.5)
            if r.status_code != 200:
                # print(f"Status Code: {r.status_code}, Response: {r.text}")
                raise ConnectionError
        except ConnectionError as e:
            # print('Error: {} Tasks: {}'.format(e, self._id))
            self.status = 'Fail'
            self.server = 'None'
            if db is not None:
                db.upsert_in_collection(dict(self), collection='tasks')

    def send_task_th(self, server='localhost', port=5000, set_time=True, sleep=0, db=None):
        send_th = Thread(target=self.send_task, args=[server, port, set_time, sleep, db])
        send_th.start()

    def send_task_results(self, server='localhost', port=5000):
        r = requests.post('http://{}:{}/task-result'.format(server, port), json=self.to_json())
        # print(f"Status Code: {r.status_code}, Response: {r.text}")

    def evaluate_requirements(self, done=True):
        if self.status == 'Processed':
            time_done = time.time()
            comp_time = time_done - self.send_time
            self.computation_time = comp_time if comp_time <= self.app.get_delay_request() else -1
            if self.computation_time == -1:
                self.status = 'Fail'
            elif done:
                self.status = 'Done'
        else:
            self.status = 'Fail'


class TaskGenerator:

    def __init__(self, app, dev='', ip='localhost'):
        self.app = app
        self.dev = dev
        self.ip = ip

    def poisson_variable(self, lamb, n=1):
        return poisson.rvs(lamb, size=n)

    """
    arrival times of tasks in a period using poisson process
    :param lamb: rate of tasks, in tasks per minute
    :param T: time period in minutes
    """
    def arrival_times(self, lamb, time_period=1.0):
        times = []
        t = expon.rvs(scale=1/lamb)
        while t < time_period:
            times.append(t)
            t += expon.rvs(scale=1/lamb)
        return np.array(times)

    def create_tasks_for_period(self, t_init_seconds=0, time_period=1.0):
        lamb = self.app.get_task_rate()
        arrival_times = self.arrival_times(lamb, time_period=time_period)
        tasks = [Task(t_init_seconds + (t*60), app=self.app, dev=self.dev, ip=self.ip) for t in arrival_times]
        return tasks

    def generate_tasks(self, sim_total_time_seconds=60, init_time=0):
        total_time_min = int(sim_total_time_seconds / 60)
        seconds_left = sim_total_time_seconds - total_time_min * 60
        tasks = []

        for t_min in range(total_time_min):
            tasks += self.create_tasks_for_period(t_min*60+init_time)

        tasks += self.create_tasks_for_period(total_time_min*60+init_time, time_period=seconds_left / 60)
        # print('Mean tasks per minute {} for app {}'.format(len(tasks)*60/sim_total_time_seconds, self.app))
        return tasks
    
def process_path(paths, devices_dict):
    if devices_dict and paths:
        all_paths = []
        for path in paths:
            path_names = []
            for node in path['links']:
                id = node['src']['device']
                path_names.append(devices_dict[id])
                if node ==  path['links'][-1]:
                    id = node['dst']['device']
                    path_names.append(devices_dict[id])
            all_paths.append(path_names)
        return all_paths
    return None
    
def onos_nodes_path(src_id, dst_id, onos_url, onos_auth):
    url = f"{onos_url}/onos/v1/paths/{src_id}/{dst_id}"
    try:
        response = requests.get(url, auth=onos_auth)
        response.raise_for_status()
        paths = response.json().get("paths", [])
        if paths:
            return paths
    except requests.exceptions.RequestException as e:
        print(f"Error querying paths: {e}")
    return None
    
def nodes_path(db, src_ip, dst_ip):
    # Get nodes path in network
    items_nodes = db.get_items_in_collection(query={'ip': {'$in': [src_ip, dst_ip]}}, collection='nodes')
    if len(items_nodes) == 2:
        items_ip = [n['ip'] for n in items_nodes]
        src_dpid = items_nodes[items_ip.index(src_ip)]['dpid']
        dst_dpid = items_nodes[items_ip.index(dst_ip)]['dpid']
        nodes_paths = onos_nodes_path(src_dpid, dst_dpid, db.onos_url, db.onos_auth)
        return nodes_paths if nodes_paths else None

# def nearest_ip(db, source_ip, sim_time=0):
#     def pos_in_time(pos, sim_t):
#         for t, p in pos:
#             if sim_t <= t:
#                 return p
#         return p
#     source_server = db.get_items_in_collection(query={'ip': source_ip}, collection='nodes')[0]
#     source_pos = np.array(pos_in_time(source_server['positions'], sim_time))

#     # Search active servers
#     server_ips = np.unique([n['ip'] for n in db.get_items_in_collection(collection='servers') if n['ip'] != source_ip and n['ip']])
#     positions = np.array([pos_in_time(db.get_items_in_collection(query={'ip': ip}, collection='nodes')[0]['positions'], sim_time)
#                 for ip in server_ips])
#     return server_ips[np.argmin(np.linalg.norm(source_pos - positions, axis=1))] if server_ips is not None and len(server_ips) > 0 else None

def nearest_edge_server(db, source_ip, sim_time=0, path=None, mecon=True, calculate_hops=False):
    source_node = db.get_items_in_collection(query={'ip': source_ip}, collection='nodes')
    # print(f"Source node {source_node}")
    conn_hist  = db.get_items_in_collection(query= {'sim_time': {'$lte': sim_time+1}}, 
    collection='connections_hist', sort=('sim_time', -1), limit=1)

    if conn_hist and source_node:
        source_node = source_node[0]
        mec_ips = []
        if mecon:
            mec_ips = np.unique([n['ip'] for n in db.get_items_in_collection(query={'mec_on': mecon}, collection='servers') if n['ip'] != source_ip])
        else:
            mec_ips = np.unique([n['ip'] for n in db.get_items_in_collection(collection='servers') if n['ip'] != source_ip and 'gn' not in n['node_name']])

        if path is not None:
            for p in path:
                mec_ips = np.delete(mec_ips, mec_ips==p)
        if len(mec_ips) > 0:
            conn_hist = conn_hist[0]
            conn_graph = conn_hist['connections']
            nodes_names = conn_hist['nodes']
            nodes_ips = conn_hist['ips']
            graph = GraphNetwork(nodes_names, conn_graph)
            distances, pred = graph.dijkstra(source_node['name'])
            distances = np.array([d if 'gg' not in nodes_names[n_i] else d+4500 for n_i, d in enumerate(distances)])

            mec_indexes = np.array([nodes_ips.index(ip) for ip in mec_ips])
            nearest_index = mec_indexes[np.argmin(distances[mec_indexes])]
            nearest_ip = np.array(nodes_ips)[nearest_index]
            hops=[]
            if calculate_hops:
                hops = [nodes_names[nearest_index], nodes_names[pred[nearest_index]]]
                last_index = pred[nearest_index]
                while(hops[-1] != source_node['name']):
                    hops.append(nodes_names[pred[last_index]])
                    last_index = pred[last_index]
                hops = list(reversed(hops))
            return nearest_ip, hops
    return None, None


def connected_nodes(db, node_name, sim_time=0):
    item = db.get_items_in_collection({'sim_time': {'$lte': sim_time}}, collection='connections_hist',
                                      sort=('sim_time', -1), limit=1)
    if len(item) > 0:
        item = item[0]
        node_index = item['nodes'].index(node_name)
        conn_row = item['connections'][node_index]
        conn_row = [v if 'st' in item['nodes'][i] else 0 for i, v in enumerate(conn_row)]
        conn_indexes = np.where(np.array(conn_row) == 1)[0]
        if len(conn_indexes) > 0:
            return np.array(item['nodes'])[conn_indexes], np.array(item['ips'])[conn_indexes]
    return None, None


def get_parameters():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--source_ip', default='10.0.0.199')
    parser.add_argument('-n', '--node_name', default=None)
    parser.add_argument('-s', '--sim_seconds', default=5400, type=int, help='Seconds of the simulation')
    parser.add_argument('-d', '--devices', default=10, type=int)
    parser.add_argument('-r', '--remote_sdn', default=False, action='store_true')
    parser.add_argument('--step', default=30, type=int)
    parser.add_argument('--seed', default=0, type=int)
    return parser.parse_args()

def sync_time():
    print('sync time')
    status = db.get_items_in_collection(collection='sim_status')[0]
    curr_time = status['sim_time']
    tu = status['time']
    tn = time.time()
    t = curr_time + int(tn-tu) + 1
    return t

if __name__ == '__main__':
    from sat_db import SatDataBase, EdgeOrchestration

    db = SatDataBase('172.17.0.1')
    args = get_parameters()
    server_ip = args.source_ip
    id_seed = int(server_ip[-2:].replace('.', ''))
    random.seed(id_seed+args.seed)

    def send_task(task, server='localhost', sleep=0, db=None):
        send_th = Thread(target=task.send_task, args=[server, 5000, True, sleep, db])
        send_th.start()

    # task_generator = TaskGenerator(App.E_HEALTH, dev='localhost')
    # heavy_comp_tasks = task_generator.generate_tasks(60*5)
    # [ht.send_task() for ht in heavy_comp_tasks]

    n_devices = args.devices
    sim_time = int(args.sim_seconds)
    all_tasks = []

    for dev_n in range(n_devices):
        task_generator = TaskGenerator(App.E_HEALTH, dev='dev{}'.format(dev_n), ip=args.source_ip)
        tasks = task_generator.generate_tasks(sim_time)
        all_tasks += tasks

    all_tasks = np.array(sorted(all_tasks, key=lambda task: task.time))
    print('Generated {} tasks for {} seconds'.format(len(all_tasks), sim_time))
    task_times = np.array([t.time for t in all_tasks])
    sent_tasks = 0

    ready = False
    sim_satus = None
    while not ready:
        sim_satus = db.get_items_in_collection(collection='sim_status')[0]
        if 'net_ready' in sim_satus.keys() and sim_satus['sim_time'] > 0 and sim_satus['net_ready']:
            print('Setting net ready to true {}'.format(sim_satus['sim_time']))
            ready = True
        else:
            print('Sleeping until network ready')
            time.sleep(1)
    # Sleep random time for each task generator
    # time.sleep((id_seed*6 + 3) % 10)

    nodes_items = db.get_items_in_collection(collection='nodes')
    devices_dict = {n['dpid']: n['name'] for n in nodes_items}

    edge_orch = EdgeOrchestration(sim_satus['orchestration'])
    mecon = (edge_orch == EdgeOrchestration.ALL_ON)
    server_ip, hops = nearest_edge_server(db, args.source_ip, 0, mecon=mecon, calculate_hops=True)

    if args.remote_sdn:
        onos_paths = nodes_path(db, args.source_ip, server_ip)
        npaths = process_path(onos_paths, devices_dict)
        npath = npaths[0] if npaths else None
    else:
        npath = hops
    print(f"First server ip: {server_ip} and npath {npath}")

    t = sync_time()

    while (t < sim_time):
        print(f"Time {t}")

        t0 = time.time()
        dt = 1
        run_tasks = all_tasks[np.where((task_times >= t) & (task_times < t+1))]
        if len(run_tasks) > 0:
            if t % 5 == 0:
                server_ip, hops = nearest_edge_server(db, args.source_ip, t, mecon=mecon, calculate_hops=True)
                print(f'Server IP {server_ip}')
                # Get nodes path
                if args.remote_sdn:
                    onos_paths = nodes_path(db, args.source_ip, server_ip)
                    npaths = process_path(onos_paths, devices_dict)
                    if npaths and np.any([p == hops for p in npaths]):
                        npath = hops
                    else:
                        print('No path found')
                        t = sync_time()
                        npath = None
                else:
                    npath = hops

                # if npath != hops:
                #     t = db.get_items_in_collection(collection='sim_status')[0]['sim_time']+1
                #     continue
            try:
                # if servers_ip is not None and len(servers_ip) > 0:
                if npath:
                    print(f'Sending tasks using path {npath}')
                    for task in run_tasks:
                        task.npath = npath
                    [send_task(task, server=server_ip, sleep=task.time-t, db=db) for task in run_tasks]
                    sent_tasks += len(run_tasks)
                    # print('sim time {}, task sent: {}, to server {}'.format(t + 1, sent_tasks, servers_ip))
            except ValueError:
                pass
        t1 = time.time()
        dt = t1 - t0
        if dt < 1:
            # print('going to sleep ', 1-dt)
            time.sleep(1-dt)
            t+=1
        else:
            t = sync_time()
















