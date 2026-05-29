from mininet.log import setLogLevel, info
from multiprocessing import Pool
from mininet.net import Mininet
from mininet.node import RemoteController, OVSController,  OVSSwitch
from mininet.link import TCLink

from network.nodes import SatNode, GndNode, GndGateway,  NodeGenerator
from comnetsemu.net import Containernet
from network.orbital_plane import OrbitalPlane
from enum import Enum
from multiprocessing import Pool
from itertools import repeat


def calculate_position(args):
    node, sim_seconds = args
    for t in range(sim_seconds + 1):
        node.calculate_position(t)

def propagate_orbit(sat_nodes, gnd_nodes, sim_seconds):
    info('Calculating coordinates for {} satellites nodes\n'.format(len(sat_nodes)))
    for n in sat_nodes:
        calculate_position((n, sim_seconds))

    info('Calculating coordinates for {} ground stations nodes\n'.format(len(gnd_nodes)))
    for n in gnd_nodes:
        calculate_position((n, sim_seconds))

    # with Pool(4) as p:
    #     info('Calculating coordinates for {} UE nodes\n'.format(len(ue_nodes)))
    #     p.map(calculate_position, [[n, sim_seconds] for n in ue_nodes])

def create_sat_network(sat_ids=(50, 100), gnd_ids=(1,),
                       tle_path='./tles/constellation_tles',
                       gnd_path='./gnd_coordinates',
                       gnd_file='Coordinates_cities.txt',
                       gateways_file='coordinates_servers.txt',
                       tle_name='TLE_Data_Starlink_Constellation.lte',
                       tle_pattern='TLE_Satellite_*.txt',
                       use_network=True,
                       remote_sdn=True,
                       use_docker=True,
                       docker_image='dev_test',
                       sat_servers=None, gnd_servers=None):
    # Create Network
    net = None
    if use_network:
        controller = RemoteController("c0", ip="127.0.0.1", port=6633, protocols="OpenFlow13") if remote_sdn else OVSController("c0") 
        # controller = 

        if use_docker:
            net = Containernet(controller=controller, link=TCLink, switch=OVSSwitch, xterms=False)

        else:
            net = Mininet(topo=None,
                      build=False,
                      ipBase='10.0.0.0/18', link=TCLink, switch=OVSSwitch)
        net.addController(controller)

    info('Add sats nodes\n')
    SatNode.docker_image = docker_image
    sat_nodes = []

    sat_servers_ids = sat_servers if sat_servers else []
    gnd_servers_ids = gnd_servers if gnd_servers else []

    other_ids = [i for i in sat_ids if i not in sat_servers]
    sat_nodes = []
    # with Pool(4) as p:
    #     sat_nodes += p.starmap(SatNode, zip(repeat(net), host_ids, repeat(tle_name), repeat(tle_pattern), repeat(tle_path)))
    # with Pool(4) as p:
    #     sat_nodes += p.starmap(SatNode, zip(repeat(net), other_ids, repeat(tle_name), repeat(tle_pattern), repeat(tle_path), repeat(False)))
    sat_nodes = [SatNode(net, i, tle_name=tle_name, tle_pattern=tle_pattern, dir_path=tle_path) for i in sat_servers_ids]
    sat_nodes += [SatNode(net, i, tle_name=tle_name, tle_pattern=tle_pattern, dir_path=tle_path, use_host=False) for i in other_ids]

    info('Add ground station nodes\n')
    GndNode.docker_image = docker_image
    gnd_nodes = [GndNode(net=net, id=i, dir_path=gnd_path, file_name=gnd_file) for i in gnd_ids]

    info('Add ground gateways\n')
    GndGateway.docker_image = docker_image
    gnd_gateways = [GndGateway(net=net, id=i, dir_path=gnd_path, file_name=gateways_file) for i in gnd_servers_ids]

    info('Calculate Orbital Planes\n')
    orbital_planes = OrbitalPlane.group_by_orbital_plane(sat_nodes)
    # orbital_planes = orbital_planes[0:2]
    [sat.compute_neighbors(orbital_planes) for sat in sat_nodes]


    # info('Add UE nodes without mininet host\n')
    # gen = NodeGenerator()
    # ue_nodes = gen.generate(n_ue)
    return net, sat_nodes, gnd_nodes, gnd_gateways
