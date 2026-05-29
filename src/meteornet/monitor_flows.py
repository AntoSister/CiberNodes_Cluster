#!/usr/bin/env python3
import time

import requests
import json
from threading import Thread
import pandas as pd
from sat_db import SatDataBase

class MonitorFlows:

    def __init__(self, ipdest=('10.0.0.1',)):
        self.ip_flow = {
            'keys': 'ipsource,ipdestination,link:inputifindex',
            'value': 'bytes',
            'log': True,
            't': 0.5,
            'n': 30,
            'flowStart': True,
            'filter': 'ipdestination={}'.format(','.join(list(ipdest)))
        }

        self.switch_flow = {
            'keys': 'link:inputifindex',
            'value': 'bytes'
        }

        self.exit_event = False
        self.mon_th = None
        self.sim_time = 0
        self.log_flows = []
        requests.put('http://localhost:8008/flow/ip_links/json', data=json.dumps(self.ip_flow))
        requests.put('http://localhost:8008/flow/s_links/json', data=json.dumps(self.switch_flow))

    # flow = {
    #     'keys': 'ipsource,ipdestination',
    #     'value': 'bytes',
    #     'filter': 'ipdestination=10.0.0.2',
    #     'log': True
    # }

    def store_flows_logs(self):
        # flowurl = 'http://localhost:8008/flows/json?maxFlows=20&timeout=10'
        flowurl = 'http://localhost:8008/activeflows/ALL/ip_links/json?timeout=10&minValue=50'

        # Connect to database
        db = SatDataBase('172.17.0.1')
        while True:
            # r = requests.get(flowurl + "&flowID=" + str(flowID))
            r = requests.get(flowurl)
            if r.status_code != 200 or self.exit_event:
                break

            flows = r.json()
            if len(flows) > 0:
                # flowID = flows[0]["flowID"]
                flows.reverse()
                sim_time = self.sim_time
                for f in flows:
                    if f['value'] > 1:
                        f['simTime'] = sim_time
                        # print(json.dumps(f))

                        # ipsource, ipdest, switch = f['flowKeys'].split(',')
                        ipsource, ipdest, switch = f['key'].split(',')
                        dict_measure = {
                            '_id': '{}_{}'.format(switch, sim_time),
                            'sim_time': sim_time,
                            'value': f['value'],
                            'data_source': f['dataSource'],
                            'ipsource': ipsource,
                            'ipdest': ipdest,
                            'switch': switch
                        }

                        db.upsert_in_collection(dict_measure, 'net_measures')

                        # self.log_flows.append([sim_time,
                        #                       f['value'],
                        #                       f['dataSource'],
                        #                       ipsource,
                        #                       ipdest,
                        #                       switch])
            time.sleep(0.5)

        # flow_df = pd.DataFrame(data=self.log_flows,
        #              columns=['sim_time', 'value', 'data_source', 'ipsource', 'ipdest', 'interface'])
        # flow_df.to_csv('data/flows/flows.csv')

    def monitor_flow(self, wait_flow=True):
        monitor_th = Thread(target=self.store_flows_logs)
        monitor_th.start()
        self.mon_th = monitor_th
        # secs = 10
        # print('Sleeping {} seconds to wait flow start'.format(secs))
        # time.sleep(secs)

        # while wait_flow and self.stored == -1 and not self.exit_event:
        #     secs = 5
        #     print('Sleeping {} seconds to wait flow start'.format(secs))
        #     time.sleep(secs)

    def exit(self):
        self.exit_event = True

    def join(self):
        if self.mon_th:
            self.mon_th.join()

