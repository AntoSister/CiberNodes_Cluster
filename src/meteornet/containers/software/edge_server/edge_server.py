import os
import time
import queue
import json
import numpy as np

from flask import Flask, request
from threading import Thread
from pymongo import MongoClient

from task_generator import Task
from task_generator import nearest_edge_server
from task_generator import connected_nodes
from sat_db import SatDataBase, EdgeOrchestration, EdgeControl
from fuzzy_control import FuzzyControl
import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers
import logging
import random


port = int(os.environ.get('SERVERPORT', 5000))
ip = os.environ.get('SERVERIP', '0.0.0.0')
cores = os.environ.get('SERVERCORES', 12)
db_ip = os.environ.get('DBIP', '172.17.0.1')
mec_on = os.environ.get('MECON', '0') == '1'
node_name = os.environ.get('NODENAME', 'st0')
edge_orchestration = EdgeOrchestration(int(os.environ.get('EDGEORCH', '0')))
edge_control = EdgeControl(int(os.environ.get('EDGECONTROL', '0')))
edge_mod = int(os.environ.get('EDGEMOD', '2'))
workers = int(os.environ.get('WORKERS', '3'))
logging.basicConfig(filename='controller.log',format='%(asctime)s %(levelname)s %(funcName)s(%(lineno)d) %(message)s', encoding='utf-8', level=logging.INFO)

class EdgeServer:
    def __init__(self, mips=800000, ram=16000000, cores=12, mec_on=True, ip='0.0.0.0', db_url='localhost', cpu_critical=90,
                 orchestration=EdgeOrchestration(0), control=EdgeControl(0), name='st'):
        self.mips = mips
        self.ram = ram
        self.cores = cores
        self.mec_on = mec_on
        self.current_instructions = 0
        self.current_ram = 0
        self.ip = ip
        self.pid = os.getpid()
        self._id = '{}_{}'.format(ip, self.pid)
        self.waiting_tasks = queue.Queue()
        self.db = SatDataBase(db_url)
        self.orchestration = orchestration
        self.control = control
        self.cpu_critical = cpu_critical
        self.node_name = name
        # TF = 0
        # TO = 1
        # PC = 2
        # CU = 3
        self.fuzzy_control = FuzzyControl(input_partitions=[[0.03, 0.1, 0.05, 0.1, 0.45, 0.15, 0.6], 
                                        [0.1, 0.2, 0.1, 0.2],
                                        [5, 10, 5, 10, 25, 25, 60],
                                        [0.05, 0.1, 0.05, 0.1]],
                       output_partition=[0.25, 0.375, 0.25, 0.5, 0.75, 0.625, 0.75])
        # FuzzyControl(input_partitions=[[0.03, 0.1, 0.05, 0.1, 0.45, 0.15, 0.6], 
        #                                 [0.1, 0.2, 0.1, 0.2],
        #                                 [5, 10, 5, 10, 25, 25, 60],
        #                                 [0.05, 0.1, 0.05, 0.1]],
        #                output_partition=[0.25, 0.375, 0.25, 0.5, 0.75, 0.625, 0.75])
        self.rl_model = self.create_model(weights='200324123815_model_weights.h5')  if self.orchestration == EdgeOrchestration.RL else None
        self.tasks_failed = 0
        self.tasks_offloaded = 0
        self.mean_cpu = 0
        self.fuzzy_output = 0
        self.constellation_usage = 0
        self.serialize_params = ['_id', 'ip', 'mec_on', 'current_instructions', 'current_ram', 'mips', 'cores', 'ram', 'node_name', 'pid']
        self.db.upsert_in_collection(dict(self), collection='servers')

    def create_model(self, num_inputs=4, num_hidden=128, num_actions=2, weights='model_weights.h5'):
        inputs = layers.Input(shape=(num_inputs, ))
        common = layers.Dense(num_hidden, activation="relu")(inputs)
        common2 = layers.Flatten()(common)
        action = layers.Dense(num_actions, activation="softmax")(common2)
        critic = layers.Dense(1)(common2)
        model = keras.Model(inputs=inputs, outputs=[action, critic])
        model.load_weights(weights)
        return model

    def set_mec(self, mec_on):
        servers_cores = self.db.get_items_in_collection(query={'ip': self.ip}, collection='servers')
        for s in servers_cores:
            s['mec_on'] = mec_on
            self.db.upsert_in_collection(s, collection='servers')

    def get_mec(self):
        servers_cores = self.db.get_items_in_collection(query={'ip': self.ip}, collection='servers')
        ret = bool(np.all([s['mec_on'] for s in servers_cores]))
        return ret

    def monitor_tasks(self):
        while True:
            task = self.waiting_tasks.get()
            self.process_task(task)
            self.waiting_tasks.task_done()

    def monitor_tasks_thread(self):
        logging.info('Starting monitor th')
        monitor_th = Thread(target=self.monitor_tasks)
        monitor_th.start()

    def update_mec_information_thread(self):
        def update_mec_info(server):
            logging.info('Starting th update info')
            step = 15
            while True:
                self.tasks_failed, self.tasks_offloaded, self.mean_cpu, self.constellation_usage, n_tasks = self.get_control_variables(step*3)
                logging.info('Got control variables {} {} {} {}'.format(self.tasks_failed, self.tasks_offloaded, self.mean_cpu, self.constellation_usage))
                # Check algorithm
                if self.orchestration == EdgeOrchestration.ACCESS_ON:
                    self.mec_on = True if n_tasks > 0 else False
                elif self.orchestration == EdgeOrchestration.FIX_ON:
                    self.mec_on = int(''.join([c for c in self.node_name if c.isdigit()])) % edge_mod == 0
                elif self.orchestration == EdgeOrchestration.ALL_ON:
                    self.mec_on = True
                elif self.orchestration == EdgeOrchestration.ALL_OFF:
                    self.mec_on = False
                elif self.orchestration == EdgeOrchestration.FUZZY:
                    self.fuzzy_output = self.fuzzy_control.predict([self.tasks_failed, self.tasks_offloaded, self.mean_cpu, self.constellation_usage])
                    logging.info('Got Fuzzy Output {}'.format(self.fuzzy_output))
                    if self.fuzzy_output != None:
                        if self.fuzzy_output <= 1/3:
                            self.mec_on = False
                        elif self.fuzzy_output >= 2/3:
                            self.mec_on = True
                    else:
                        self.fuzzy_output = 0.5
                elif self.orchestration == EdgeOrchestration.RL:
                    state = tf.expand_dims([self.tasks_failed, self.tasks_offloaded, self.mean_cpu, self.constellation_usage], 0)
                    action_probs, critic_value = self.rl_model(state)
                    action_probs = np.squeeze(action_probs)
                    logging.info('actions {}'.format(action_probs))
                    self.mec_on = False if action_probs[0] > action_probs[1] else True
                elif self.orchestration == EdgeOrchestration.RANDOM:
                    random_choice = True if random.random() > 0.5 else False
                    self.mec_on = random_choice
                if self.orchestration != EdgeOrchestration.REMOTE:
                    logging.info('Setting mec to : {}'.format(self.mec_on))
                    self.set_mec(self.mec_on)
                self.update_state(0, 0)
                self.calculate_server_usage(control_loop=True)
                time.sleep(step)

        logging.info('Starting th update info 1')
        update_th = Thread(target=update_mec_info, args=(self,))
        update_th.start()
    
    def connected_nodes(self):
        sim_time = self.db.get_items_in_collection(collection='sim_status')[0]['sim_time']
        _, conn_nodes = connected_nodes(self.db, self.node_name, sim_time)
        return conn_nodes

    def run_task(self, task):
        time_to_process = task.length / self.mips
        logging.info('Time to process task {} : {}'.format(task, time_to_process))
        time.sleep(time_to_process)

        self.update_state(-task.length, -task.container_size)
        task.status = 'Processed'
        task.send_task_results(server=task.ip)

    def process_local(self, task):
        self.update_state(task.length, task.container_size)
        run_th = Thread(target=self.run_task, args=[task, ])
        run_th.start()

    def task_failed(self, task):
        task.status = 'Fail'
        self.db.upsert_in_collection(dict(task), collection='tasks')
        # task.send_task_results(server=task.ip)

    def offload_task_control(self, task):
        if self.control == EdgeControl.OFF:
            # Only offload if cpu usage is critical
            sim_time = self.db.get_items_in_collection(collection='sim_status')[0]['sim_time']
            near_server, _ = nearest_edge_server(self.db, self.ip, sim_time, task.path)
            if near_server is not None:
                logging.info('Offloading Task to {}'.format(near_server))
                task.send_task_th(near_server, set_time=False)
                return True
            return False
        elif self.control == EdgeControl.FUZZY:
            # TODO: program fuzzy control
            return False
        elif self.control == EdgeControl.Q_LEARNING:
            # TODO: program q learning algorithm
            return False

    def process_task(self, task):
        cpu_usage, ram_usage = self.calculate_server_usage()
        logging.info('Current usage, cpu: {}, ram, {}'.format(cpu_usage, ram_usage))
        task.server = self.ip

        # int(100*current_instructions/(self.mips*self.cores)), int(current_ram/self.ram)

        offloaded = False
        estimate_cpu_usage = (cpu_usage if cpu_usage is not None else 0) + int(task.length * 100 / (self.mips*self.cores))
        server_can_process = estimate_cpu_usage <= self.cpu_critical
        if server_can_process:
            if self.get_mec():
                logging.info('Processing task locally')
                self.process_local(task)
                return
        
        # Offload task
        offloaded = self.offload_task_control(task)

        logging.info('Offload task ? {}'.format(offloaded))
        if not offloaded:
            conn_nodes = self.connected_nodes()
            if not server_can_process and conn_nodes is not None:
                not_pass = [c not in task.path for c in conn_nodes]
                conn_index = 0
                if np.any(not_pass):
                    conn_index = not_pass.index(True)
                task.send_task_th(conn_nodes[conn_index], set_time=False)
                return
            logging.info('Task failed: {}'.format(task))
            self.task_failed(task)

    def update_state(self, instructions, ram):
        self.current_instructions += instructions
        self.current_ram += ram
        self_dic = dict(self)
        del self_dic['mec_on']
        self.db.upsert_in_collection(self_dic, collection='servers')

    def __iter__(self):
        for key in self.serialize_params:
            yield key, self.__dict__[key] if isinstance(self.__dict__[key], int) or isinstance(self.__dict__[key], float) else str(self.__dict__[key])

    def to_json(self):
        return json.dumps(dict(self))

    def get_database(self, db_name='sat_net', db_url='localhost'):
        # Provide the mongodb atlas url to connect python to mongodb using pymongo
        CONNECTION_STRING = "mongodb://{}:27017/SatNet".format(db_url)
        # Create a connection using MongoClient. You can import MongoClient or use pymongo.MongoClient
        client = MongoClient(CONNECTION_STRING)
        # Create the database for our example (we will use the same database throughout the tutorial
        return client['{}'.format(db_name)]
    

    def get_control_variables(self, step=60):
        time_now = time.time()
        tasks =  self.db.get_items_in_collection(collection='tasks', 
                                                 query={'send_time': {'$gt': time_now-step}})
        # cpu_hist = self.db.get_items_in_collection(collection='servers_hist', query={'name': self.ip,
        #                                                                   'mec': True,
        #                                                                    'time': {'$gt': time_now-step}})
        cpu_hist_all = self.db.get_items_in_collection(collection='servers_hist',
                                                        query={'mec': True, 'time': {'$gt': time_now-step}})
        cpu_hist = [c for c in cpu_hist_all if c['name'] == self.ip]
        
        
        active_servers = list(np.unique([v['name'] for v in cpu_hist_all]))
        all_servers = list(np.unique([v['ip'] for v in self.db.get_items_in_collection(collection='servers') if 'st' in v['node_name'] or 'gg' in v['node_name'] ]))
        if self.ip in active_servers:
            active_servers.remove(self.ip)
            all_servers.remove(self.ip)

        cu = len(active_servers) / len(all_servers) if len(all_servers) != 0 else 0

        logging.info('Cpu Hist')
        cpu = int(np.percentile([c['cpu'] for c in cpu_hist], 70)) if len(cpu_hist) > 0 else 0
        logging.info('Cpu is {}'.format(cpu))
        tasks = [t for t in tasks if self.ip in t['path'] and t['server']  != 'None']

        tf = 0
        to = 0
        if len(tasks) > 0:
            failed_tasks = [ t for t in tasks if t['status'] == 'Fail']
            # print('Offloaded tasks', [ [ip for ip in t['path'] if ip != t['ip']] for t in tasks])
            # print('Offloaded tasks', [ t for t in tasks if self.ip != [ip for ip in t['path'] if ip != t['server']][-1]])
            offload_tasks = [ t for t in tasks if self.ip != [ip for ip in t['path'] if ip != t['ip']][-1]]
            # print('Offloaded tasks', len(offload_tasks))

            tf = len(failed_tasks) / len(tasks)
            to = len(offload_tasks) / len(tasks)
        return tf, to, cpu, cu, len(tasks)


    def calculate_server_usage(self, control_loop=False):
        sim_status = self.db.get_items_in_collection(collection='sim_status')[0]
        mec_on = self.get_mec()
        if control_loop:
            logging.info('Got mec of {} class {} control loop {}'.format(mec_on, self._id, control_loop))
        if mec_on:
            items = self.db.get_items_in_collection(query={'ip': self.ip}, collection='servers')
            current_instructions = sum([it['current_instructions'] for it in items])
            current_ram = sum([it['current_ram'] for it in items])
            cpu_usage, ram_usage = int(100*current_instructions/(self.mips*self.cores)), int(current_ram/self.ram)
        else:
            cpu_usage = None
            ram_usage = None
        self.db.insert_in_collection({'name': self.ip,
                                      'time': time.time(),
                                      'sim_time': sim_status['sim_time'],
                                      'mec': mec_on,
                                      'cpu': cpu_usage, 'ram': ram_usage,
                                      'tasks_failed': self.tasks_failed, 'tasks_offloaded': self.tasks_offloaded,
                                      'mean_cpu': self.mean_cpu,
                                      'constellation_usage': self.constellation_usage,
                                      'control_loop' : control_loop,
                                      'fuzzy_output': self.fuzzy_output}, collection='servers_hist')
        return cpu_usage, ram_usage


edge_server = EdgeServer(ip=ip, db_url=db_ip, cores=int(cores), mec_on=mec_on,
                         orchestration=edge_orchestration, control=edge_control, name=node_name)
logging.info(f'EdgeServer id {edge_server._id}, ip {edge_server.ip}')
edge_server.monitor_tasks_thread()
items = edge_server.db.get_items_in_collection(query={'ip': edge_server.ip}, 
                                               collection='servers')
logging.info(f'Number of workers {workers}')
while len(items) != workers:
    logging.info(f'Len Items {len(items)}')
    time.sleep(1)
    items = edge_server.db.get_items_in_collection(query={'ip': edge_server.ip}, 
                                                   collection='servers')
workers_id = [i['_id'] for i in items]
workers_id.sort()

if workers_id[0] == edge_server._id:
    edge_server.update_mec_information_thread()
app = Flask(__name__)


@app.route("/")
def server_info():
    return "Server Info:\n {}".format(dict(edge_server))


@app.route('/process-task', methods=['POST'])
def process_task():
    try:
        data = request.json
        task = Task.from_json(json.loads(data), ip)
        edge_server.waiting_tasks.put(task)

        return 'OK'
    except Exception as e:
        logging.info('Error task process {}, {}'.format(e, data))
        return 'ERROR'


@app.route('/task-result', methods=['POST'])
def task_result():
    try:
        data = request.json
        task = Task.from_json(json.loads(data), ip)
        task.evaluate_requirements(done=True)
        edge_server.db.upsert_in_collection(dict(task), collection='tasks')
        return 'OK'
    except Exception as e:
        print('Error on task result {}, {}'.format(e, data))
        return 'ERROR'


if __name__ == "__main__":
    logging.info('Hola edge server')
    app.run(host=ip, port=port)
