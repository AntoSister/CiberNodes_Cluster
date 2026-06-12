import sys
import argparse
import numpy as np
import re
import time
import gzip
import os
import subprocess
import pandas as pd
import logging
from multiprocessing import Pool
from pathlib import Path
from numpy import random

# MODIFICADO: Bloque de importación protegida para Mininet
try:
    from mininet.log import setLogLevel, info
    from mininet.term import makeTerms
except ImportError:
    # Si estamos en el cluster (Bare Metal), Mininet no está instalado.
    # Definimos funciones vacías para que el resto del código no falle al llamarlas.
    def setLogLevel(*args, **kwargs): pass
    def info(msg): logging.info(msg)
    def makeTerms(*args, **kwargs): return []

from sflow import config_switches
from monitor_flows import MonitorFlows
from sat_db import SatDataBase, EdgeControl, EdgeOrchestration
from network.orbit import propagate_orbit, create_sat_network
from network.graph import GraphNetwork
from network.nodes import GndGateway

# MODIFICADO: Bloque de importación protegida para Comnetsemu
try:
    from comnetsemu.clean import cleanup
    from comnetsemu.clean import sh
except ImportError:
    # Definimos funciones dummy para evitar errores de importación
    def cleanup(): pass
    def sh(cmd, **kwargs): return ""

from logging.handlers import RotatingFileHandler


def save_table_cache(table, node, nue, step, sim_total_time_s):
    cache_dir = Path('data/cache')
    base_name = '{}/{}_{}_{}_{}_cache'.format(cache_dir, node.name, nue, step, sim_total_time_s)
    table.to_csv('{}.xz'.format(base_name))

def get_table_cache(node, nue, step, sim_total_time_s):
    cache_dir = Path('data/cache')
    base_name = '{}/{}_{}_{}_{}_cache'.format(cache_dir, node.name, nue, step, sim_total_time_s)
    return pd.read_csv('{}.xz'.format(base_name))

    # calculate_contact_tables(sat_nodes, ue_nodes, args.sim_seconds, step=args.step, save_cache=True)
def save_status(db, sim_time, timestamp, net_ready=None, nodes=None, n_sat=None, 
                n_gnd=None, n_ggs=None, devices=None, orchestration=None, total_seconds=None):
    status = {'_id': 'sys', 'sim_time': sim_time}
    if net_ready is not None:
        status['net_ready'] = net_ready
    if n_sat is not None:
        status['n_sat'] = n_sat
    if n_gnd is not None:
        status['n_gnd'] = n_gnd
    if n_ggs is not None:
        status['n_ggs'] = n_ggs
    if devices is not None:
        status['devices'] = devices
    if orchestration is not None:
        status['orchestration'] = orchestration
    if total_seconds is not None:
        status['total_seconds'] = total_seconds
    status['time'] = timestamp

    db.upsert_in_collection(status, collection='sim_status')
    if nodes is not None:
        graph = GraphNetwork(nodes)
        connection_matrix = graph.graph_array
        connections = {'_id': sim_time, 'sim_time': sim_time,
                        'nodes': [n.name for n in nodes],
                       'ips': [n.host.IP() if n.host else '' for n in nodes],
                       'connections': connection_matrix.tolist()}
        db.upsert_in_collection(connections,
                                collection='connections_hist',
                                id='sim_time')

# def calculate_network_adjacency(nodes):
#     adjacency_matrix = np.zeros((len(nodes), len(nodes)))
#     nodes_map = { n: i for i, n in enumerate(nodes)}
#     nodes_aux = nodes.copy()

#     for i_n, ns in enumerate(nodes):
#         nodes_aux.remove(ns)
#         connections = [1 if c else 0 for c in ns.check_connection(nodes_aux)]
#         for nt, conn in zip(nodes_aux, connections):
#             adjacency_matrix[nodes_map[ns], nodes_map[nt]] = conn
#             adjacency_matrix[nodes_map[nt], nodes_map[ns]] = conn
#             # if conn == 1:
#             #     print(nodes_map[ns], ns.name, nodes_map[nt], nt.name, conn)
#     return adjacency_matrix

def calculate_contact_tables(src_nodes, other_nodes,  sim_total_time_s, step,  save_cache=False):
    tables = [node.compute_contact_table(other_nodes, sim_total_time_s, step) for node in src_nodes]
    if save_cache:
        for table, node in zip(tables, src_nodes):
            save_table_cache(table, node, len(other_nodes), step, sim_total_time_s)
    return tables

def manage_graph_links(nodes_d, graph):
    nodes_restart = set()
    for link_k in graph.keys():
        n1_name, n2_name =  link_k.split('-')
        n1, n2 = nodes_d[n1_name], nodes_d[n2_name]
        # Check link
        new_nodes = n1.enable_link(n2, True, graph[link_k])
        nodes_restart = nodes_restart.union(new_nodes)
    return nodes_restart

def manage_links(curr_node, contact_table, sat_nodes, t, multilink=False, use_sflow=None, step=20):
    new_contacts = [sat for sat in sat_nodes if sat.name in contact_table.keys() and contact_table.loc[int(t/step)][sat.name] > 0]
    new_contacts_dist = [contact_table.loc[int(t/step)][sat.name] for sat in sat_nodes if sat.name in contact_table.keys()
                         and contact_table.loc[int(t/step)][sat.name] > 0]
    curr_contacts = [sat_nodes[i_s] for i_s, conn in enumerate(curr_node.check_connection(sat_nodes)) if conn]
    links_changed = set()
    # if there are not new contact deactivate all previous ones
    if len(new_contacts) < 1:
        for node in curr_contacts:
            links_changed = curr_node.enable_link(node, False)
    # if there are new contacts but are not current contacts
    elif len([nc for nc in new_contacts if nc in curr_contacts]) < 1:
        # deactivate lost contacts previously connected
        for node in curr_contacts:
            links_changed = links_changed.union(curr_node.enable_link(node, False))

        for n_contact, nc_index in enumerate(np.argsort(new_contacts_dist)):
            if multilink or n_contact == 0:
                links_changed = links_changed.union(
                    curr_node.enable_link(new_contacts[nc_index], True, new_contacts_dist[nc_index]))
    # if there are new_contact in current ones
    else:
        for node, dist in zip(new_contacts, new_contacts_dist):
            if multilink or node in curr_contacts:
                links_changed = links_changed.union(curr_node.enable_link(node, True, dist))
            # else:
            #     if node in curr_contacts:
            #         curr_node.enable_link(node, True, dist)

    if len(links_changed) > 0:
        if use_sflow:
            config_switches(sat_net, '127.0.0.1', 'lo')
                        # topo={'nodes': {'ue2s': {'agent': '127.0.0.1', 'ports': {'ue2s-eth1': {'ifindex': '52'}, 'ue2s-eth2': {'ifindex': '71'}}}},'links': {'ue2s-sat10s': {'node1': 'ue2s', 'port1': 'ue2s-eth2', 'node2': 'sat10s', 'port2': 'sat10s-eth2'}}})
        # sat_net.send_topology()

def make_conn_graph(nodes, conn_tables):
    conn_dict = {}
    for node, table in zip(nodes, conn_tables):
        n1 = node.name
        other_nodes = table.keys()
        for n2 in other_nodes:
            if '{}-{}'.format(n1, n2) not in conn_dict and '{}-{}'.format(n2, n1) not in conn_dict:
                conn_dict.update({'{}-{}'.format(n1, n2) :table[n2]})

    return conn_dict, {node.name : node  for node in nodes}


def run_simulation(sat_nodes, gnd_nodes,
                   contact_tables, sky_tables, sim_total_time_s, db,
                   step=20, speed_factor=1, init_time=0, update_status=True, multilink=False):
    logging.info('Run Simulation\n')
    for t in range(init_time, sim_total_time_s+step, step):

        # if use_sflow:
        #     use_sflow.sim_time = t

        logging.info('Simulation Time: {} seconds\n'.format(t))
        # old_way = False

        t0 = time.time()

        # if old_way:
        #     for gnd, table in zip(gnd_nodes, contact_tables):
        #         manage_links(gnd, table, sat_nodes, t, use_sflow=use_sflow, step=step, multilink=multilink)
        #     t_gl = time.time()
        #     logging.info('Time to create glinks: {} seconds'.format(t_gl - t0))

        #     # Manage ISL
        #     for sat, table in zip(sat_nodes, sky_tables):
        #         # isl_table = table[[st_node.name for st_node in sat_nodes if st_node.name in table.keys()]]
        #         isl_table = table.loc[:, table.keys()[1:]]
        #         manage_links(sat, isl_table, sat_nodes, t, True, use_sflow=use_sflow, step=step, multilink=multilink)
        #     t_isl = time.time()
        #     logging.info('Time to create isl: {} seconds'.format(t_isl - t_gl))
        # else:
            # [contact_table.loc[int(t/step)][sat.name] for sat in sat_nodes if sat.name in contact_table.keys()
            #                         and contact_table.loc[int(t/step)][sat.name] > 0]
        gnd_tables_t, gnd_contacts = [], []
        for table, g_node in zip(contact_tables, gnd_nodes):
            table_t = table.loc[int(t/step)]
            table_t = table_t[table_t.keys()[1:]]
            table_t = table_t[table_t > 0]
            if len(table_t) > 0:
                if not multilink and not isinstance(g_node, GndGateway):
                    # select an active link if there is one
                    maintain_link = False
                    active_links = list(g_node.active_links)
                    for a_link in active_links:
                        if a_link.name in table_t.keys():
                            table_t = table_t[[a_link.name]]
                            maintain_link = True
                            break
                        else:
                            g_node.enable_link(a_link, False)
                    if not maintain_link:
                        table_t = table_t[table_t == table_t.min()]
                else:
                    # Remove active links not in table
                    active_links = list(g_node.active_links)
                    for a_link in active_links:
                        if a_link.name not in table_t.keys():
                            g_node.enable_link(a_link, False)

                gnd_tables_t.append(table_t)
                gnd_contacts.append(g_node)
            else:
                active_links = list(g_node.active_links)
                for node in active_links:
                    g_node.enable_link(node, False)


        sky_tables_t =  [table.loc[int(t/step), table.columns[1:]] for table in sky_tables]
        nodes = gnd_contacts + sat_nodes
        tables_t = gnd_tables_t + sky_tables_t
        conn_graph, nodes_dict = make_conn_graph(nodes, tables_t)
        nodes_restart = manage_graph_links(nodes_dict, conn_graph)
        for node in nodes_restart:
            os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {node.name}")
                # os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {node.name}")
        for node in nodes_restart:
                logging.info(f'Restarting switch {node.name}')
                node.restart_switch()

        if update_status:
            save_status(db, t, t0, net_ready=True, nodes=sat_nodes+gnd_nodes)
        t1 = time.time()
        if speed_factor != -1:
            delay = step/speed_factor - (t1-t0)
            logging.info('Delay: {} , diff: {}\n'.format( delay, (t1-t0)))
            if delay > 0:
                time.sleep(delay)

def clean_constellation():
    links = sh("ip link show")
    ret = re.findall(r"((st[\d]+|gn[\d]+|gg[\d]+)h?-(st[\d]+|gn[\d]+|gg[\d]+)h?)", links)
    if ret:
        for link in ret:
            sh("ip link delete {}".format(link[0]), check=False)
    # Change .Xauthority permissions
    clean_xauth()

def get_parameters():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--tle_path', default='./tles/constellation_tles', help='Path to read TLEs configuration files')
    parser.add_argument('-u', '--ue_path', default='./Coordinates', help='Path to read UEs configuration files')
    parser.add_argument('-s', '--sim_seconds', default=100, type=int, help='Seconds of the simulation')
    parser.add_argument('-i', '--init_second', default=0, type=int, help='Init second of simulation')
    parser.add_argument('-x', '--speed_factor', default=1, type=int, help='Accelerate simulation time by speed factor')
    parser.add_argument('-d', '--docker', default=False, action='store_true', help='Use docker containers to simulate hosts')
    # Argumento para desactivar Mininet/Docker y usar procesos locales directamente
    parser.add_argument('--bare_metal', default=False, action='store_true', help='Use bare metal processes instead of containers/mininet')
    parser.add_argument('-r', '--remote_sdn', default=False, action='store_true', help='Choose if use remote SDN')
    parser.add_argument('-c', '--clean', default=False, action='store_true')
    # parser.add_argument('--sflow', default=False, action='store_true', help='Activate sflow metrics')
    parser.add_argument('--sats', nargs='+', default=[7, 8 , 9], type=int, help='List of satellites TLEs to read')
    parser.add_argument('--sat_servers',nargs='*', default=[], type=int, help='List of sats with hosts')
    parser.add_argument('--gnd_servers',nargs='*', default=[], type=int, help='List of ground gateways')
    parser.add_argument('--docker_image', default='edge_server', help='Image to use in docker hosts')
    parser.add_argument('--step', default=30, type=int, help='Contact table step in seconds')
    parser.add_argument('--gnds', nargs='+', default=[1], type=int, help='List of ground devices')
    parser.add_argument('--edge_orchestration', default=2, type=int)
    parser.add_argument('--edge_control', default=0, type=int)
    parser.add_argument('--edge_modulo', default=2, type=int)
    parser.add_argument('--devices', default=100, type=int, help='Number Devices per groundstation')
    parser.add_argument('--client', default=False, action='store_true', help='Start Mininet client at simulation end')
    parser.add_argument('--gnd_file', default='Coordinates_cities_ordered.txt', help='File name of GND coordinates')
    parser.add_argument('--ggs_file', default='ksat_coordinates.txt', help='File name of Gateways coodinates')
    parser.add_argument('--seed', default=0, type=int)
    parser.add_argument('--multilink', default=False, action='store_true',
                        help='Allow simultaneous links to multiple satellites (overlapping handover)')

    return parser.parse_args()

def clean_xauth():
    sudo_user = os.environ.get("USER", None)
    if os.path.exists(f"/home/{sudo_user}/.Xauthority"):
        subprocess.run(f"chown {sudo_user}:{sudo_user} /home/{sudo_user}/.Xauthority",
            check=True,
            shell=True,
            stderr=subprocess.DEVNULL,
        )

def clean_exit(mon_flows=None, terms=[]):
    cleanup()
    clean_constellation()
    # if sat_net:
    #     sat_net.stop()
    if mon_flows:
        mon_flows.exit()
        mon_flows.join()
    for t in terms:
        t.kill()


def setup_logging():
    logfile = 'sat_network.log'
    rotating_handler = RotatingFileHandler(logfile, mode='a', maxBytes=500 * 1024 * 1024,
                                    backupCount=2, encoding=None, delay=0)
    level = logging.INFO
    rotating_handler.setLevel(level)

    logging.basicConfig(format='%(asctime)s %(levelname)s %(funcName)s(%(lineno)d) %(message)s', level=level,
        handlers=[rotating_handler, logging.StreamHandler()])
    logging.info('Setting Rotating Logging')

    app_log = logging.getLogger('root')
    app_log.setLevel(level)


if __name__ == "__main__":

    # Parse parameters
    args = get_parameters()

    if args.clean:
        print('cleaning')
        clean_exit()
        exit()

    setup_logging()

    random.seed(0)

    sats = [s for s in args.sats if s !=-1] if len(args.sats) != 1 else [i for i in range(1, args.sats[0]+1)]
    gnds = args.gnds


    setLogLevel("info")
    # setLogLevel("critical")
    sat_net = None
    mon_flows = None
    terms = []
    # MODIFICADO: Usamos localhost (127.0.0.1) si es Bare Metal, de lo contrario usamos la IP del bridge de Docker
    db_ip = '127.0.0.1' if args.bare_metal else '172.17.0.1'
    db = SatDataBase(db_ip)
    db.drop_db()
    try:
        # Create network topology
        # MODIFICADO: Desactivamos la creación de red virtual y contenedores si estamos en modo bare_metal
        sat_net, sat_nodes, gnd_nodes, gnd_gateways = create_sat_network(sat_ids=sats, gnd_ids=gnds,
                                                                       tle_path=args.tle_path,
                                                                       gnd_file=args.gnd_file,
                                                                       tle_name=None,
                                                                       tle_pattern='TLE_*_{}.txt',
                                                                       use_network=not args.bare_metal, # No crear red Mininet si es bare_metal
                                                                       use_docker=args.docker and not args.bare_metal, # No usar Docker si es bare_metal
                                                                       docker_image=args.docker_image,
                                                                       sat_servers=args.sat_servers, 
                                                                       gnd_servers=args.gnd_servers,
                                                                       gateways_file=args.ggs_file,
                                                                       remote_sdn=args.remote_sdn)

        # Calculate satellites orbits and ground stations positions
        propagate_orbit(sat_nodes, gnd_nodes+gnd_gateways, args.sim_seconds)

        # Calculate contact tables from ground
        info('Calculate contact tables from ground stations\n')
        ground_contact = calculate_contact_tables(gnd_nodes+gnd_gateways, sat_nodes,
                                                  args.sim_seconds+args.step, step=args.step,
                                                  save_cache=True)

        # if not args.use_cache:
            # Calculate ISL contact tables and contacts with ue
        info('Calculate ISL and UE contact tables\n')
        sky_contact = calculate_contact_tables(sat_nodes, [], 
                                               args.sim_seconds+args.step, step=args.step,
                                               save_cache=True)
        # else:
        #     # Calculate ISL contact tables and contacts with ue
        #     info('Calculate ISL and UE contact tables\n')
        #     sky_contact = [get_table_cache(s, 0, args.step, args.sim_seconds) for s in sat_nodes]

        run_simulation(sat_nodes, gnd_nodes+gnd_gateways,
                       ground_contact, sky_contact, args.init_second, db,
                       speed_factor=-1, init_time=args.init_second,
                       step=args.step, update_status=False, multilink=args.multilink)
        # MODIFICADO: Solo iniciamos la red si sat_net no es None (no es Bare Metal)
        if sat_net:
            sat_net.start()
        logging.info('Network started\n')
        mon_flows = None
        # if args.sflow:
        #     config_switches(sat_net, '127.0.0.1', 'lo')
        #     mon_flows = MonitorFlows(ipdest=[n.host.IP() for n in sat_nodes if n.host] + [n.host.IP() for n in gnd_gateways if n.host])
        #     mon_flows.monitor_flow()
        for n in gnd_nodes+gnd_gateways:
            # n.host.cmd('/home/sflow-rt/start.sh &')
            # MODIFICADO: Usamos localhost si no hay red virtual de Mininet
            n_ip = n.host.IP() if n.host else '127.0.0.1'
            logging.info('Ground Node {} has IP {}\n'.format(n.name, n_ip))
            # print(dict(n))
            db.upsert_in_collection(dict(n), 'nodes')
        for n in sat_nodes:
            # n.host.cmd('./sandbox/sflow-rt/start.sh &')
            if n.host:
                logging.info('Sat Node {} has IP {}\n'.format(n.name, n.host.IP()))
            else:
                # MODIFICADO: Lanzar el software real (SuchaiFS + HoneySat) en modo Bare Metal
                logging.info('Sat Node {} - Lanzando procesos Bare Metal (SuchaiFS + HoneySat)\n'.format(n.name))
                n.run_satellite() # <-- Ejecuta el software de vuelo y física
            db.upsert_in_collection(dict(n), 'nodes')

        save_status(db, 0, time.time(), n_sat=len(sat_nodes), n_gnd=len(gnd_nodes), n_ggs=len(gnd_gateways), 
                    devices=args.devices, orchestration=args.edge_orchestration, total_seconds=args.sim_seconds,
                    nodes=sat_nodes+gnd_nodes+gnd_gateways)
        edge_orch = EdgeOrchestration(args.edge_orchestration)
        edge_control = EdgeControl(args.edge_control)
        edge_server_ths =[]

        if args.docker:
            from comnetsemu.cli import CLI, spawnXtermDocker

            #terms = [spawnXtermDocker(str(n.host)) for n in sat_nodes]
            terms = []
            edge_server_ths = [n.run_edge_server(cores=12, mecon=False, edge_orchestration=EdgeOrchestration.ALL_OFF,
                                                 edge_mod=args.edge_modulo, name=n.name, workers=2) for n in gnd_nodes] 
            
            servers_ths = []
            host_workers =  2 if len(args.sat_servers+args.gnd_servers) > 5 else 4
            for n in sat_nodes+gnd_gateways:
                if n.host:
                    servers_ths.append(n.run_edge_server(cores=24, mecon=False, edge_orchestration=edge_orch,
                                                    edge_control=edge_control,
                                                    edge_mod=args.edge_modulo, name=n.name, workers=host_workers))
                    wait_seconds = 7
                    logging.info('Waiting {} secs for starting edge server {} \n'.format(wait_seconds, n.name))
                    time.sleep(wait_seconds)       

            edge_server_ths += servers_ths

        # Ping between hosts to promote discovery
        all_host_nodes = [n for n in gnd_nodes+gnd_gateways+sat_nodes if n.host]
        ping_ths = []
        for i, node_src in enumerate(all_host_nodes):
            for j, node_dst in enumerate(all_host_nodes):
                if i == j or j < i:
                    continue
                # MODIFICADO: Solo intentamos ping si ambos nodos tienen un host de Mininet
                if node_src.host and node_dst.host:
                    ping_ths.append(node_src.ping(node_dst))

        if args.docker:
            for n in gnd_nodes:
                n.cmd('python3 task_generator.py -n {} -i {} -s {} -d {} --seed {} {} > tasks.logs 2>&1 &'.format(n.name, n.host.IP(),
                        args.sim_seconds, args.devices, args.seed, '-r' if args.remote_sdn else ''))
        else:
            # MODIFICADO: Solo intentamos abrir terminales xterm si NO es bare_metal
            if not args.bare_metal:
                from mininet.cli import CLI
                terms = makeTerms([n.host for n in gnd_nodes if n.host])
                terms += makeTerms([n.host for n in sat_nodes if n.host])
                terms += makeTerms([n.host for n in gnd_gateways if n.host])
        sleep_secs = 10
        for t in range(sleep_secs):
            logging.info('Sleeping {} secs to start everything, {}/{} \n'.format(sleep_secs, t+1, sleep_secs))
            # if t+1 == 15:
            time.sleep(1)
        
        # [n.cmd('ping 10.0.0.2 -w {} > ping.logs 2>&1 &'.format(args.sim_seconds)) for n in gnd_nodes]
        init_time = args.init_second + 1 if args.sim_seconds > args.init_second else args.sim_seconds
        save_status(db, init_time, time.time(), net_ready=True, nodes=sat_nodes+gnd_nodes+gnd_gateways)
        run_simulation(sat_nodes, gnd_nodes+gnd_gateways,
                       ground_contact, sky_contact, args.sim_seconds, db,
                       speed_factor=args.speed_factor, init_time=init_time, step=args.step,
                       multilink=args.multilink)

        for t in terms:
            t.wait()

        if args.client and not args.bare_metal: # MODIFICADO: No lanzamos la consola si no hay red virtual
            CLI(sat_net)

        clean_exit(mon_flows, terms)
    except Exception as e:
        # cleanup()
        # clean_constellation()
        clean_exit(mon_flows, terms)
        logging.error('Exception catch {}'.format(e))
        raise e
    except KeyboardInterrupt:
        # cleanup()
        clean_exit(mon_flows, terms)
        logging.error('KeyboardInterrupt catched')
        sys.exit(0)
