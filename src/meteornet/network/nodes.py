import time

from mininet.node import CPULimitedHost, Host, Node
from comnetsemu.net import Containernet
from pathlib import Path
from sgp4.api import Satrec, WGS72, jday, days2mdhms
from threading import Thread
from sat_db import EdgeControl, EdgeOrchestration
from pyproj import Transformer

import datetime
import math
import json
import numpy as np
import pandas as pd
import numbers
import logging
import os
import time
import subprocess # NUEVO: Para lanzar procesos en el SO en modo Bare Metal


class NetNode:
    name = None
    switch = None
    dpid = None
    host = None
    docker_image = 'dev_test'
    link_parameters = None
    # light speed constant in km/s
    light_speed = 2.998e+5
    used_ids = []
    id = None
    serialize_params = ['_id', 'id', 'name', 'dpid']

    def __init__(self, net=None, use_host=True, host_delay=1):
        self.net = net
        self.cache = {'positions': [], 'contacts': []}
        self.neighbors = []
        self.active_links = set()
        self.name = '{}{}'.format(self.base_name(), self.id)
        self._id = self.name
        if self.net:
            self.switch = net.addSwitch('{}'.format(self.name), failMode='standalone', protocols="OpenFlow13")
            self.dpid = f'of:{self.switch.dpid}'
            # self.switch = net.addSwitch('{}'.format(self.name), failMode='standalone', stp=True)
            ip = "10.0.{}.{}".format(int(self.id / 255), self.id % 255)
            if use_host:
                if self.use_docker():
                    self.host = net.addDockerHost('{}h'.format(self.name), dimage=self.docker_image, ip=ip,
                                                docker_args={})
                else:
                    self.host = net.addHost('{}h'.format(self.name), cls=Host, ip=ip)
                self.add_link(self.host, self.switch, delay=host_delay)

    def change_link_parameters(self, node, distance):
        connections = self.link().connectionsTo(node.link())
        if len(connections) > 0:
            bw, delay = self.calculate_link_parameters(node, distance)

            src_link = connections[0][0]
            dst_link = connections[0][1]

            if src_link.params != dst_link.params or src_link.params['bw'] != bw or abs(int(src_link.params['delay'].replace('ms', '')) - delay) > 8:
                logging.info('Changing parameters of link between {} and {} from {} to {} {}'.format(node.name, self.name, src_link.params, bw, delay))
                src_link.config(**{'bw': bw, 'delay': "{}ms".format(delay)})
                src_link.params['bw'] = bw
                src_link.params['delay'] = '{}ms'.format(delay)
                dst_link.config(**{'bw': bw, 'delay': "{}ms".format(delay)})
                dst_link.params['bw'] = bw
                dst_link.params['delay'] = '{}ms'.format(delay)
                time.sleep(0.1)

    def calculate_link_parameters(self, node, distance):
        link_parameters = self.link_parameters
        if not isinstance(node, SatNode):
            link_parameters = node.link_parameters
        th_distances = link_parameters[:, 0]
        link_index = np.searchsorted(th_distances, distance)
        if link_index == len(th_distances):
            link_index -= 1
        # Bandwidth in Mbps
        bw = link_parameters[link_index, 1]
        # Delay in ms
        delay = int(1 + (distance / self.light_speed) * 1e3)
        return bw, delay

    def add_link(self, n1, n2, bw=15, delay=1):
        if self.use_docker():
            return self.net.addLinkNamedIfce(n1, n2, bw=bw, delay="{}ms".format(delay))
        else:
            return self.net.addLink(n1, n2, bw=bw, delay="{}ms".format(delay))

    def add_active_link(self, node):
        self.active_links.add(node)

    def remove_active_link(self, node):
        self.active_links.remove(node)

    def enable_link(self, node,  enable, distance=None):
        out_link = self.check_link(node)
        nodes_changed = set()
        if enable:
            # Create Interface if not exist
            if not out_link[0]:
                logging.info('Adding new connection up between {} and {}'.format(self.link(), node.link()))
                # os.system(f'ovs-vsctl del-controller {node.link()}')
                # os.system(f'ovs-vsctl del-controller {node.link()}')

                # logging.info(f'{self.link().connectionsTo(node.link())}')
                bw, delay = self.calculate_link_parameters(node, distance)
                self.add_link(self.link(), node.link(), bw=bw, delay=delay)
                time.sleep(0.1)
                # new_link.interf1.ifconfig('up')
                self.add_active_link(node)
                node.add_active_link(self)
                # os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {self.name}")
                # os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {node.name}")
                # self.restart_switch()
                # node.restart_switch()
                nodes_changed = {self, node}
            # Create logical link if link is down
            elif not out_link[1]:
                logging.info('Old Connection up between {} and {}'.format(self.link(), node.link()))
                # logging.info(f'{self.link().connectionsTo(node.link())}')
                # self.net.configLinkStatus(self.link().name, node.link().name, 'up')
                conns1  = self.link().connectionsTo(node.link())
                conns2  = node.link().connectionsTo(self.link())

                for conn in conns1 + conns2:
                    while(not conn[0].isUp() and not conn[1].isUp()):
                        self.net.configLinkStatus(self.link().name, node.link().name, 'up')
                        self.net.configLinkStatus(node.link().name, self.link().name, 'up')
                        # os.system(f"sudo ip link set {conn[0].name} up")
                        # os.system(f"sudo ip link set {conn[1].name} up")

                # self.restart_switch()
                # node.restart_switch()

                # os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {node.link()}")
                # os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {self.link()}")

                self.change_link_parameters(node, distance)

                self.add_active_link(node)
                node.add_active_link(self)
                # links_changed = {self, node}
            # Change bandwidth and delay according to distance
            elif distance is not None:
                self.change_link_parameters(node, distance)
                # self.restart_switch()
                # node.restart_switch()
            # if len(links_changed) > 0:
                # logging.info('Links changed: {}'.format([l.name for l in links_changed]))
        elif not enable:
            if out_link[1]:
                os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {self.link().name}")
                os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {node.link().name}")
                # conns1  = self.link().connectionsTo(node.link())
                # conns2  = node.link().connectionsTo(self.link())
                # self.net.delLinkBetween(self.link().name, node.link().name, allLinks=True)
                delete_links = []
                for link in self.net.links:
                    inf1 , inf2 = link.intf1, link.intf2
                    n1, n2 = inf1.name.split('-')
                    if (n1 == self.name and n2 == node.name) or (n2 == self.name and n1 == node.name):
                        delete_links.append(link)
                for link in delete_links:
                    self.net.delLink(link)

                
                # connections = self.link().connectionsTo(node.link())[0]

                # for conn in conns1:
                #     self.net.delLink(conn[0])
                #     self.net.delLink(conn[1])
                    # time.sleep(0.1)
                        # os.system(f"sudo ip link set {conn[0].name} down")
                        # os.system(f"sudo ip link set {conn[1].name} down")
                
                # os.system(f"ip link delete {self.link().name}-{node.link().name}")
                # os.system(f"ip link delete {node.link().name}-{self.link().name}")

                os.system(f'ovs-vsctl del-port {self.link().name} {self.link().name}-{node.link().name}')
                os.system(f'ovs-vsctl del-port {node.link().name} {node.link().name}-{self.link().name}')
                time.sleep(0.2)
                
                self.remove_active_link(node)
                node.remove_active_link(self)
                nodes_changed = {self, node}
                logging.info('Connection down between {} and {}, got outlink'.format(self.link(), node.link()))
            # conns = self.link().connectionsTo(node.link())
            # logging.info(f'ConnectionsTo {conns} {[[l.isUp() for l in con] for con in conns ]}')
        return nodes_changed

    def restart_switch(self):
        if self.switch:
            # keep connections status to recover after the restart
            interfaces = self.switch.intfList()
            status = [intf.isUp() for intf in interfaces]
            self.switch.start(self.net.controllers)
            # os.system(f"sudo ovs-ofctl del-flows -O OpenFlow13 {self.name}")
            for intf, stat in zip(interfaces, status):
                if not stat:
                    n1 ,n2 = intf.name.split('-')
                    self.net.configLinkStatus(n1, n2, 'down')
                    # self.net.configLinkStatus(n2, n1, 'down')
            time.sleep(0.3)

    def check_connection(self, nodes):
        return [self.check_link(n)[1] for n in nodes]

    def check_link(self, node):
        connections = self.link().connectionsTo(node.link())
        interface, status = False, False
        if len(connections) > 0:
            interface = True
            status = connections[0][0].isUp() and connections[0][1].isUp()
        return interface, status

    def link(self):
        return self.switch

    def get_position(self, sim_seconds=0):
        cache_len = len(self.cache['positions'])
        if cache_len <= sim_seconds:
            self.cache['positions'] = self.cache['positions'] + [None]*(sim_seconds-cache_len) + [self.calculate_position(sim_seconds)]
        elif self.cache['positions'][sim_seconds] is None:
            self.cache['positions'][sim_seconds] = self.calculate_position(sim_seconds)
        return self.cache['positions'][sim_seconds]

    def use_docker(self):
        return isinstance(self.net, Containernet)

    def base_name(self):
        return 'nt'

    def get_id(self, id=None):
        if self.id is None:
            if id is not None:
                if id in NetNode.used_ids:
                    self.id = np.max(NetNode.used_ids)+1
                    NetNode.used_ids.append(self.id)
                else:
                    self.id = id
                    NetNode.used_ids.append(self.id)
            else:
                self.id = np.max(NetNode.used_ids)+1 if len(NetNode.used_ids) > 0 else 1
                NetNode.used_ids.append(self.id)
        return self.id

    def run_edge_server(self, cores=12, mecon=False,
                        edge_orchestration=EdgeOrchestration.ACCESS_ON, edge_control=EdgeControl.OFF,
                        edge_mod=2, name=name, workers=1):
        # self.cmd('FLASK_IP={} python3 edge_server.py &'.format(self.host.IP()))
        edge_cmd = f'export SERVERIP={self.host.IP()} SERVERCORES={cores} MECON={1 if mecon else 0} EDGEORCH={edge_orchestration.value} EDGECONTROL={edge_control.value} EDGEMOD={edge_mod} NODENAME={name} WORKERS={workers} && sh -c "gunicorn --bind $SERVERIP:5000 --workers={workers} --timeout=300  wsgi:app > server.log  2>&1 &"'
        print(f'edge server cmd {edge_cmd}')
        return self.cmd(edge_cmd)

    def ping(self, node, timeout=5):
        if node.host and self.host:
            ping_cmd = f'ping -c 1 -W {timeout} {node.host.IP()}'
            print(f'ping command {ping_cmd}')
            return self.host.cmd(ping_cmd)

    def cmd(self, command):
        """
        Ejecuta un comando en el nodo.
        - Modo Original: Usa el host de Mininet (contenedor o host virtual).
        - Modo Bare Metal: Si no hay host, lanza un proceso real en el cluster usando subprocess.
        """
        run_cmd = None
        if self.host:
            # Si existe un host de Mininet, lo usamos (comportamiento original)
            run_cmd = Thread(target=self.host.cmd, args=[command, ])
            run_cmd.start()
        else:
            # NUEVO: Si estamos en Bare Metal (no hay host), usamos subprocess del SO
            def run_os_cmd(cmd):
                logging.info(f"MODO BARE METAL - Ejecutando comando real: {cmd}")
                # Popen lanza el proceso en segundo plano sin bloquear MeteorNet
                subprocess.Popen(cmd, shell=True)
            
            run_cmd = Thread(target=run_os_cmd, args=[command, ])
            run_cmd.start()
            
        return run_cmd

    def serialize_cache(self):
        return {'positions': [[i_p, list(pos)] for i_p, pos in enumerate(self.cache['positions']) if pos is not None],
                'contacts': self.cache['contacts']}

    def __iter__(self):
        for key in self.serialize_params:
            if isinstance(self.__dict__[key], (int, float, str)):
                yield key, self.__dict__[key]
            elif isinstance(self.__dict__[key], numbers.Number):
                yield key, self.__dict__[key].item()
            else:
                yield key, dict(self.__dict__[key])
        yield 'ip', self.host.IP() if self.host else ''
        cache = self.serialize_cache()
        for key in cache.keys():
            yield key, cache[key]

    def to_json(self):
        return json.dumps(dict(self))

    # Abstract function
    def calculate_position(self, sim_seconds=0):
        return None

    @staticmethod
    def sats_in_sight_from_point(sat_coords, gnd_coord):
        plane_normal = gnd_coord / np.linalg.norm(gnd_coord)
        s_in_sight = NetNode.right_of_plane(gnd_coord, plane_normal, sat_coords)
        return s_in_sight

    @staticmethod
    def right_of_plane(origin, normal, point):
        v = point - origin
        determinant = np.dot(normal, v)
        return determinant >= 0

class Epoch():
    e = None
    r = None
    v = None
    def __init__(self, jdsatepoch, jdsatepochF, epochyr, epochdays, inclo, nodeo, argpo, mo):
        self.jdsatepoch = jdsatepoch
        self.jdsatepochF = jdsatepochF
        self.epochyr = epochyr
        self.epochdays = epochdays
        self.inclo = inclo
        self.nodeo = nodeo
        self.argpo = argpo
        self.mo = mo

class SatNode(NetNode):
    def __init__(self, net=None, id=1, tle_name=None, tle_pattern=None, dir_path='constellation_tles', use_host=True):
        self.link_parameters = np.array([
            [500, 14],
            [1000, 14],
            [2000, 14],
            [3000, 14],
            [4000, 14]
        ])
        self.init_coords, self.init_date = self.read_tle(tle_name, tle_pattern, dir_path=dir_path, sat_id=id)
        self.get_id(id)
        self.coords = self.init_coords
        # self.name = '{}{}'.format(self.base_name(), id) if self.name is None else self.name
        self.orbital_plane = None
        super().__init__(net, use_host=use_host)

    def set_orbital_plane(self, orbital_plane):
        if np.linalg.norm(self.calculate_plane_normal() - orbital_plane.normal) < 1E-2:
            self.orbital_plane = orbital_plane
            orbital_plane.add_if_in_plane(self)
            return True
        return False
    
    def satrec(self):
        return Satrec.twoline2rv(self.tle[0], self.tle[1])


    def read_tle(self, tle_name=None, tle_pattern='TLE_*_{}.txt', dir_path='constellation_tles', sat_id=1):
        tles_dir = Path(dir_path)
        glob_patt = tle_pattern.format(sat_id) if not tle_name else tle_name
        tle_file = list(tles_dir.glob(glob_patt))[0]
        fp = open(tle_file)
        two_lines = ''
        if not tle_name:
            two_lines = [fp.readline().replace('\n', ''), fp.readline().replace('\n', '')]
        else:
            lines = fp.readlines()
            two_lines = lines[(sat_id-1)*3+1:(sat_id-1)*3+3]
        
        self.tle = (two_lines[0], two_lines[1])
        # sat = 
        # self.sgp4 = sat.sgp4
        # self.sat = sat

        init_coords, init_date = self.initial_conditions()
        return init_coords, init_date

    def initial_conditions(self):
        sat = self.satrec()
        self.epoch = Epoch(sat.jdsatepoch, sat.jdsatepochF, sat.epochyr, sat.epochdays, sat.inclo, sat.nodeo, sat.argpo, sat.mo)
        e, r, v = sat.sgp4(sat.jdsatepoch, sat.jdsatepochF)
        self.epoch.e, self.epoch.r, self.epoch.v = e, r, v
        init_coords = np.array(r)
        init_date = datetime.datetime(*np.array((2000+sat.epochyr,) + days2mdhms(sat.epochyr, sat.epochdays)).astype(int))
        return init_coords, init_date

    def calculate_position(self, sim_seconds=0):
        return self.propagate_orbit(sim_seconds)

    def calculate_lonlat(self, sim_seconds=0):
        r_ecef = self.calculate_ecef_position(sim_seconds)
        transformer = Transformer.from_crs("epsg:4978", "epsg:4326", always_xy=True)  # ECEF to WGS84
        x, y, z = r_ecef * 1000  # Convert km to meters
        lon, lat, alt = transformer.transform(x, y, z)
        return np.array([lon, lat])


    def gmst(self, jd_ut1):
        T = (jd_ut1 - 2451545.0) / 36525.0
        gmst_sec = 67310.54841 + (876600 * 3600 + 8640184.812866) * T + \
                0.093104 * T**2 - 6.2e-6 * T**3
        gmst_sec = gmst_sec % 86400  # Wrap around 24h
        if gmst_sec < 0:
            gmst_sec += 86400
        gmst_rad = 2 * np.pi * (gmst_sec / 86400.0)
        return gmst_rad

    def calculate_ecef_position(self, sim_seconds=0):
        curr_date = self.init_date + datetime.timedelta(seconds=sim_seconds)
        jd, fr = jday(curr_date.year, curr_date.month, curr_date.day, curr_date.hour, curr_date.minute,
                      curr_date.second)
        r_teme = self.propagate_orbit(sim_seconds)
        theta = self.gmst(jd + fr)
        rotation_matrix = np.array([
            [ np.cos(theta), np.sin(theta), 0],
            [-np.sin(theta), np.cos(theta), 0],
            [             0,             0, 1]
        ])
        r_ecef = rotation_matrix @ r_teme
        return r_ecef

    def calculate_plane_normal(self):
        e, r, v = self.epoch.e, self.epoch.r, self.epoch.v
        r, v = r / np.linalg.norm(r), v / np.linalg.norm(v)
        n = np.cross(r, v)
        return n

    def propagate_orbit(self, sim_seconds):
        sat = self.satrec()
        curr_date = self.init_date + datetime.timedelta(seconds=sim_seconds)
        jd, fr = jday(curr_date.year, curr_date.month, curr_date.day, curr_date.hour, curr_date.minute,
                      curr_date.second)
        self.coords = np.array(sat.sgp4(jd, fr)[1])
        return self.coords

    def compute_contact_table(self, gnd_nodes, total_seconds, step=1):
        logging.info('Calculating contact table for {} , with {} nodes for {} seconds'.format(self.name, len(gnd_nodes), total_seconds))
        data = [self.contact_with_nodes(gnd_nodes, sec) for sec in range(0, total_seconds+1,  step)]
        [self.cache['contacts'].append([d['sim_time'], [[k, v] for k, v in d.items() if v > 0 and k != 'sim_time']]) for d in data]
        return pd.DataFrame(data)

    def contact_with_nodes(self, gnd_nodes, sim_seconds):
        # Ground contacts
        gnd_coords = [node.get_position(sim_seconds) for node in gnd_nodes]
        gnd_contact_data = self.contact_with_ground(gnd_coords, sim_seconds)
        gnd_contact_columns = [n.name for n in gnd_nodes]
        # Inter links contacts
        sky_coords = [node.get_position(sim_seconds) for node in self.neighbors]
        sky_contact_data = self.contact_with_sky(sky_coords, sim_seconds, -1)
        sky_contact_columns = [n.name for n in self.neighbors]

        contact_dict = {'sim_time': sim_seconds}
        for key, data in zip(sky_contact_columns + gnd_contact_columns, sky_contact_data + gnd_contact_data):
            contact_dict[key] = data
        return contact_dict

    def contact_with_sky(self, sat_coords, sim_seconds, th_distance=-1):
        coord = self.get_position(sim_seconds)
        distances = [np.linalg.norm(sat_coord - coord) for sat_coord in sat_coords]
        return [d if d < th_distance or th_distance == -1 else 0 for d in distances]

    def contact_with_ground(self, gnd_coords, sim_seconds):
        sat_coord = self.get_position(sim_seconds)
        contacts = [NetNode.sats_in_sight_from_point(sat_coord, gnd_coord) for gnd_coord in gnd_coords]
        return [np.linalg.norm(sat_coord - gnd_coords[i_s]) if c else 0 for i_s, c in enumerate(contacts)]

    def compute_neighbors(self, orbital_planes):
        # print(self.id)
        if self.orbital_plane is None:
            add_to_plane = False
            for plane in orbital_planes:
                if self.set_orbital_plane(plane):
                    add_to_plane = True
                    break
            if not add_to_plane:
                # TODO: Create a new plane since is not in the list provided.
                pass
        if self.orbital_plane is not None:
            # Get neighbors in the same orbital plane
            neighbors = self.orbital_plane.get_neighbors(self)
            # print('id', self.id, 'neighbors', [n.id for n in neighbors])
            # Get nearest planes
            nearest_planes = self.orbital_plane.get_nearest_planes(orbital_planes)
            for pln in nearest_planes:
                nearest_sat = pln.get_nearest_sat(self)
                if nearest_sat:
                    neighbors.append(nearest_sat)

            self.neighbors = neighbors

    def base_name(self):
        return 'st'


class GndNode(NetNode):
    def __init__(self, latitude=None, longitude=None, altitude=None, net=None, id=1,
                 dir_path='gnd_coordinates', file_name='Coordinates_cities.txt', 
                 use_host=True, host_delay=1):
        self.equatorial_rad = 6378.137
        self.polar_rad = 6356.755
        self.link_parameters = np.array([
            [500, 14],
            [1000, 14],
            [2000, 14],
            [3000, 14],
            [4000, 14]
        ])
        self.latitude = latitude
        self.longitude = longitude
        self.altitude = altitude if altitude is not None else 0
        if self.latitude is None or self.longitude is None:
            dir = Path(dir_path)
            fp = open(list(dir.glob(file_name))[0])
            gnd_line = fp.readlines()[id-1]
            lat_lon_alt = [float(n) for n in gnd_line.replace('\n', '').split(' ')]
            self.latitude, self.longitude, self.altitude = lat_lon_alt

        if net is not None:
            self.get_id(id)
        else:
            self.id = id


        self.serialize_params = self.serialize_params + ['latitude', 'longitude', 'altitude']
        super().__init__(net, use_host=use_host, host_delay=host_delay)
        self.init_coords = self.get_position()
        self.coords = self.init_coords

    def calculate_position(self, sim_seconds=0):
        latitude = self.latitude * math.pi / 180
        longitude = self.longitude * math.pi / 180

        geocentric_latitude = math.atan((self.polar_rad / self.equatorial_rad) * math.tan(latitude))
        earth_rad = self.altitude + ((self.equatorial_rad * self.polar_rad) / math.sqrt(
            pow(self.equatorial_rad * math.sin(geocentric_latitude), 2) + pow(self.polar_rad * math.cos(geocentric_latitude), 2)))
        ue_coord = np.zeros(3)
        rot_longitude = longitude + (2 * math.pi / 86400) * (sim_seconds + 1)
        ue_coord[0] = earth_rad * math.cos(rot_longitude) * math.cos(geocentric_latitude)
        ue_coord[1] = earth_rad * math.sin(rot_longitude) * math.cos(geocentric_latitude)
        ue_coord[2] = earth_rad * math.sin(geocentric_latitude)
        self.coords = ue_coord
        return self.coords

    def compute_contact_table(self, sat_nodes, total_seconds, step=1):
        data = [self.contact_with_constellation(sat_nodes, sec) for sec in range(0, total_seconds+1, step)]
        [self.cache['contacts'].append([d['sim_time'], [[k, v] for k, v in d.items() if v > 0 and k != 'sim_time']])
         for d in data]
        return pd.DataFrame(data)

    def contact_with_constellation(self, sat_nodes, sim_seconds):
        sat_coords = [node.get_position(sim_seconds)  for node in sat_nodes]
        contact_data = self.contact_with_sky(sat_coords, sim_seconds)
        contact_columns = [n.name for n in sat_nodes]
        contact_dict = {'sim_time': sim_seconds}
        for key, data in zip(contact_columns, contact_data):
            contact_dict[key] = data
        return contact_dict

    def contact_with_sky(self, sat_coords, sim_seconds):
        gnd_coord = self.get_position(sim_seconds)
        contacts = [NetNode.sats_in_sight_from_point(sat_coord, gnd_coord)for sat_coord in sat_coords]
        return [np.linalg.norm(gnd_coord - sat_coords[i_s]) if c else 0 for i_s, c in enumerate(contacts)]

    def calculate_geo_distance(self, des_node):
        R = 6373.0
        lat1 = math.radians(self.latitude)
        lon1 = math.radians(self.longitude)
        lat2 = math.radians(des_node.latitude)
        lon2 = math.radians(des_node.longitude)


        dlon = lon2 - lon1
        dlat = lat2 - lat1

        a = math.sin(dlat / 2)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2)**2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
        distance = R * c
        return distance
        
    def base_name(self):
        return 'gn'
    
class GndGateway(GndNode):
    def __init__(self, latitude=None, longitude=None, altitude=None, net=None, id=1, 
                 dir_path='gnd_coordinates', file_name='coordinates_gateways.txt', host_delay=13):
        super().__init__(latitude, longitude, altitude, net, id, dir_path, file_name, use_host=True, host_delay=host_delay)

    def base_name(self):
        return 'gg'


class NodeGenerator:

    def __init__(self, net=None, dir_path='gnd_coordinates', file_name='worldcities.csv', source_class=GndNode):
        self.net = net
        self.dir_path = dir_path
        self.source_class = source_class
        self.file_name = file_name

    def generate(self, n_nodes, start_id=0):
        generated_nodes = []
        if Path(self.file_name).suffix == '.csv':
            df_locations = pd.read_csv('{}/{}'.format(self.dir_path, self.file_name))
            lat_lon = df_locations[['lat', 'lng']].iloc[:n_nodes].to_numpy()
            generated_nodes = [self.source_class(latitude=lat_lon[id][0], longitude=lat_lon[id][1], altitude=0.0, id=start_id+id)
                               for id in range(n_nodes)]
        return generated_nodes



