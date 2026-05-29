import sys
import numpy as np
from pathlib import Path
import argparse
import pyvista as pv
import datetime
import time
import os
import pandas as pd
os.environ["QT_API"] = "pyqt5"

from qtpy import QtWidgets

from pyvistaqt import BackgroundPlotter
from pyvistaqt import QtInteractor, MainWindow
from pyvista import examples

from threading import Thread
from network.orbit import propagate_orbit, create_sat_network
from sat_db import SatDataBase
from plots.plot_database import get_dataset
from network.nodes import GndGateway
from network.graph import GraphNetwork

class PlotConstellation(MainWindow):

    def __init__(self, parent=None):
        QtWidgets.QMainWindow.__init__(self, parent)

        # create the frame
        self.frame = QtWidgets.QFrame()
        vlayout = QtWidgets.QVBoxLayout()

        # add the pyvista interactor object
        self.plotter = QtInteractor(self.frame)
        vlayout.addWidget(self.plotter.interactor)
        self.signal_close.connect(self.plotter.close)

        self.frame.setLayout(vlayout)
        self.setCentralWidget(self.frame)
        # self.plotter = BackgroundPlotter()

        equatorial_rad = 6378.137
        polar_rad = 6356.755
        mean_rad = (equatorial_rad + polar_rad) / 2.0

        # earth_pd = pv.Sphere(mean_rad)
        earth_pd = examples.load_globe().scale(10E-7)
        texture = examples.load_globe_texture()
        
        # self.plotter.add_mesh(earth_pd, style='surface', opacity=0.7, color='lightgray')
        self.plotter.add_mesh(earth_pd, texture=texture, smooth_shading=True, opacity=1.0)
        self.plotter.add_axes()
        # self.plotter.show_bounds(grid='back', location='outer')
        self.actors = []
        self.show()
        self.plotter.suppress_rendering = True
        

    def clear(self):
        for actor in self.actors:
            self.plotter.remove_actor(actor)

    def plot_coords(self, sat_nodes, gnd_nodes, ue_nodes, init_time, end_time,
                    sat_servers, gnd_servers, tasks=None, show_glinks=False):
        self.clear()
        self.plotter.suppress_rendering = True

        font_size = 33
        sh_op = 0.5
        sh_color = 'white'
        node_size = 15
        ue_size = 40
        host_size = 40
        isl_link_width= 1
        g_link_width = 5
        sat_color = 'silver'
        isl_col = 'tab:orange'
        host_color='tab:blue'
        gn_col = 'tab:green'
        glink_col = 'tab:cyan'
        always_v = True

        tg_names = [f'gn{g.id+len(sat_nodes)}' for g in gnd_nodes if 'gn' in g.name]

        sim_tasks = [t for t in tasks if t['time'] <= end_time and t['time'] >= init_time]
        # network_paths = [[n for n in t['npath'].replace("'", "").replace(']', '').replace('[', '').split(', ') if 'st'  in n] for t in sim_tasks if t['npath'] != 'None']
        network_paths = [[n for n in t['npath'].replace("'", "").replace(']', '').replace('[', '').split(', ') if 'st' in n] for t in sim_tasks if t['npath'] != 'None' and np.any([tg in t['npath'] for tg in tg_names])]
        # measures = []

        mean_time = int((init_time + end_time) / 2.0)

        for i_s, s in enumerate(sat_nodes):
            # lines = []
            for n in s.neighbors:
                line = [sat_nodes[i_s].get_position(mean_time),
                        n.get_position(mean_time)]
                # value = sum([m['value'] for m in measures if '{}-{}'.format(n.name, s.name) == m['switch'] or '{}-{}'.format(s.name, n.name) == m['switch']]) if measures else 0

                value = sum([ 1 if abs((p.index(s.name) if s.name in p else -100) - (p.index(n.name) if n.name in p else -100)) == 1 else 0  for p in network_paths])

                isl_col = 'powderblue' if value > 0 else 'tab:orange'
                isl_link_width= 15 if value > 0 else 1
                
                self.actors.append(self.plotter.add_lines(np.array(line), color=isl_col, width=isl_link_width))
                

        for gnd_node in gnd_nodes:
            min_d = 0
            contact_dict = gnd_node.contact_with_constellation(sat_nodes, mean_time)
            lines =[]
            for key in contact_dict.keys():
                if contact_dict[key] != 0:
                    if 'gn' in gnd_node.name:
                        if min_d != 0 and contact_dict[key] >= min_d:
                            continue
                        else:
                            lines=[]
                        
                    if key in [s.name for s in sat_nodes]:
                        min_d = contact_dict[key]
                        s_node = sat_nodes[[s.name for s in sat_nodes].index(key)]
                        line = [s_node.get_position(mean_time),
                                gnd_node.get_position(mean_time)]
                        lines += line
            if lines and len(lines) > 0 and show_glinks:
                line_actor = self.plotter.add_lines(np.array(lines), 
                                    color=glink_col, width=g_link_width)
                self.actors.append(line_actor)

        # add gnd devices

        if gnd_nodes:
            pdata_gnd = pv.PolyData([n.get_position(mean_time)for n in gnd_nodes if not isinstance(n, GndGateway)])
            pdata_gnd['ground'] = [n.name.replace('gn', 'u') for n in gnd_nodes if not isinstance(n, GndGateway)]
            self.actors.append(
                self.plotter.add_mesh(pdata_gnd, point_size=ue_size, render=False, reset_camera=False, color=gn_col)
            )

            self.actors.append(
                    self.plotter.add_point_labels(pdata_gnd, 'ground', show_points=False,
                                            fill_shape=True, render=False, reset_camera=False, always_visible=always_v,
                                                pickable=True, shape_opacity=sh_op, font_size=font_size, point_size=1.0, shape_color=sh_color)
            )


            if gnd_servers:
            # add gndgateway
                pdata_gnd = pv.PolyData([n.get_position(mean_time)for n in gnd_nodes if isinstance(n, GndGateway)])
                gnd_servers_map = {int(j):i+1 for i, j in enumerate(gnd_servers)}
                pdata_gnd['ground'] = [f'g{gnd_servers_map[n.id]}' for n in gnd_nodes if isinstance(n, GndGateway)]
                if len(pdata_gnd['ground']) > 0:
                    self.actors.append(
                        self.plotter.add_mesh(pdata_gnd, point_size=host_size, render=False, reset_camera=False, color=host_color)
                    )

                    self.actors.append(
                            self.plotter.add_point_labels(pdata_gnd, 'ground', show_points=False,
                                                    fill_shape=True, render=False, reset_camera=False, always_visible=always_v,
                                                        pickable=True, shape_opacity=sh_op, font_size=font_size, point_size=1.0)
                    )

            
        
        n_sats = len(sat_nodes)
        # sat_colors = plt.cm.get_cmap("tab10", 100).colors[:, :3]
        for i_sat in range(n_sats):
            pdata = pv.PolyData(sat_nodes[i_sat].get_position(mean_time))

            pdata['sat'] = [i_sat+1]
            if sat_servers and  sat_nodes[i_sat].id in sat_servers:
                    self.actors.append(
                        self.plotter.add_points(pdata, point_size=host_size, color='gold',
                                        render_points_as_spheres=True, render=False, reset_camera=False)
                    )

                    pdata['sats'] = ['{}'.format(sat_nodes[i_sat].name.replace('t', ''))]

                    self.actors.append(
                        self.plotter.add_point_labels(pdata, 'sats', show_points=False,
                                                fill_shape=True, render=False, reset_camera=False, always_visible=always_v,
                                                    pickable=True, shape_opacity=sh_op, font_size=font_size, point_size=1.0, shape_color=sh_color)
                    )
            else:
                s_color = sat_color
                s_size = node_size

                if sat_nodes[i_sat].name in np.unique(sum(network_paths, [])):
                    s_color = 'gold'
                    s_size = node_size*2

                
                self.actors.append(
                    self.plotter.add_points(pdata, point_size=s_size, color=s_color, 
                                render_points_as_spheres=True, render=False, reset_camera=False)
                )

        if ue_nodes and len(ue_nodes) > 0:
            pdata_ue = pv.PolyData([n.get_position(mean_time)for n in ue_nodes])
            # pdata_ue['ground ue'] = [i+1 for i in range(len(ue_nodes))]
            self.actors.append(
                self.plotter.add_mesh(pdata_ue, point_size=node_size, render=False, reset_camera=False)
            )

        def clicked(event):
            self.plotter.save_graphic('sat_servers.svg')

        self.plotter.track_click_position(callback=clicked, side='right', viewport=True)
        self.plotter.suppress_rendering = False
        self.plotter.update()
        
        # self.plotter.remove_scalar_bar('sat')
        # self.plotter.remove_scalar_bar('ground')
        # self.plotter.add_scalar_bar(title='Ground Station', vertical=True, n_colors=2)
        # self.plotter.remove_scalar_bar('ground')
        # self.plotter.scalar_bars['ground'].set
        # self.plotter.add_legend([['gn11', 'blue'], ['gn12', 'red']])
        # plotter.show_grid()
        # self.plotter.update()


def distance(n1, n2, time=0):
    return np.sqrt(((n1.calculate_position(time) - n2.calculate_position(time))**2).sum())

def get_parameters():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tle_path', default='./tles/constellation_tles', help='Path to read TLEs configuration files')
    parser.add_argument('--gnd_path', default='./gnd_coordinates', help='Path to read UEs configuration files')
    parser.add_argument('--gnd_file', default='Coordinates_cities_ordered.txt', help='File name of GND coordinates')
    parser.add_argument('--ggs_file', default='coordinates_servers.txt', help='File name of Gateways coodinates')
    parser.add_argument('--sats', nargs='+', default=[7, 8, 9], type=int, help='List of satellites TLEs to read')
    parser.add_argument('-s', '--sim_seconds', default=60, type=int, help='Seconds of the simulation')
    parser.add_argument('--ground_contacts', action='store_true', default=False)
    parser.add_argument('--gnds', nargs='+', default=[1], type=int, help='List of ground nodes')
    parser.add_argument('--render_video', action='store_true', default=False)
    # parser.add_argument('--net_dataset', default='space_vs_ground/ground/5ms/100sats_2gnd_20dev_2700seconds_2orch_17-11-24_05-45-49', help='')
    parser.add_argument('--sat_servers',nargs='*', default=[], type=int, help='List of sats with hosts')
    parser.add_argument('--gnd_servers',nargs='*', default=[], type=int, help='List of ground gateways')

    return parser.parse_args()


def draw_loop(plotter, sim_seconds, sat_nodes, gnd_nodes, ue_nodes, sat_servers, gnd_servers, tasks, step=60):
    init_time = 0 if sim_seconds < step else sim_seconds - step
    plotter.plot_coords(sat_nodes, gnd_nodes, ue_nodes, init_time, sim_seconds, sat_servers, gnd_servers, tasks)



if __name__ == '__main__':
    pv.set_plot_theme('document')
    args = get_parameters()
    db = SatDataBase('172.17.0.1')


    sats = [s for s in args.sats if s !=-1] if len(args.sats) != 1 else [i for i in range(1, args.sats[0]+1)]

    
    sat_servers = args.sat_servers
    gnd_servers = args.gnd_servers
    # gnd_servers = [i+1 for i in range(28)]

    # # Space Cloud Neighbor Orbitals
    # sat_servers = "1 12 23 34 45 56 67 78 89 100".split(" ")
    # gnd_servers = [] 

    # # Neighbors planes
    # sat_servers = "1 2 3 4 5 6 7 8 9 10".split(' ')
    # gnd_servers = [] 


    # # Farthest Sampling
    # sat_servers = [int(s) for s in "160 56 154 39 96 103 10 3 123 180 99 53 7".split(' ')]
    # gnd_servers = []

    # # All gg

    # sat_servers = [160, 159, 158, 157]
    # sat_servers = [72, 73, 62, 51, 40, 29, 18, 7, 8]

    # gnd_servers = [int(i) for i in "12 24 19 17 8 26 16 14 9 4".split(' ')]


        # hosts_ids = [148, 159, 158, 157, 156, 167, 178]
        # print(f'host ids len {len(hosts_ids)}')
        # print(hosts_ids)
    
    gnds = args.gnds if  len(args.gnds) > 0 and args.gnds[0] != -1 else []
    net, sat_nodes, gnd_nodes, gnd_gateways = create_sat_network(sat_ids=sats, gnd_ids=gnds, tle_path=args.tle_path, 
                                                             gnd_path=args.gnd_path, gnd_file=args.gnd_file, tle_name=None, tle_pattern='TLE_*_{}.txt',
                                                             use_network=False, sat_servers=sat_servers, gnd_servers=gnd_servers, gateways_file=args.ggs_file)

    sat_nodes = sorted(sat_nodes, key=lambda sat: sat.id)
    sat_data = np.array([[s.name] + list(s.calculate_lonlat()) + [0, s.id] for s in sat_nodes])
    distances_ue = [np.linalg.norm(s.calculate_position()-np.array([g.calculate_position()for g in gnd_nodes]), axis=1).mean() for s in sat_nodes]
    print(f'Satellite {sat_nodes[np.argmin(distances_ue)].name} has lower distance to ue' )
    # df = pd.DataFrame(sat_data, columns=['name', 'x', 'y'])
    # df.to_csv('sat_nodes_198.csv')

    # ue_data = np.array([[g.name] + [g.longitude, g.latitude] + [gnd_gateways[0].calculate_geo_distance(g)] + [ g.id] for g in gnd_nodes])
    # df_ue = pd.DataFrame(ue_data, columns=['name', 'x', 'y', 'dis', 'id'])
    # df_ue.to_csv('ue_nodes.csv', index=False)

    # fps_ids = [f'st{id}' for id in "160 56 154 39 96 103 10 3 123 180".split(' ')]
    # fps_data = [d for d in sat_data if d[0] in fps_ids]
    # df_fps = pd.DataFrame(fps_data, columns=['name', 'x', 'y', 'z', 'id'])
    # df_fps.to_csv('fps_nodes.csv', index=False)

    # cp_ids = [f'st{id}' for id in "6 28 50 72 94 105 116 138 160 193".split(' ')] 
    # cp_data = [d for d in sat_data if d[0] in cp_ids]
    # df_cp = pd.DataFrame(cp_data, columns=['name', 'x', 'y', 'z', 'id'])
    # df_cp.to_csv('cp_nodes.csv', index=False)

    # ao_ids = [f'st{id}' for id in "156 157 158 159 160 161 162 163 164 165".split(' ')] 
    # ao_data = [d for d in sat_data if d[0] in ao_ids]
    # df_ao = pd.DataFrame(ao_data, columns=['name', 'x', 'y', 'z', 'id'])
    # df_ao.to_csv('ao_nodes.csv', index=False)

    # graph = GraphNetwork(sat_nodes)
    # selected_nodes = graph.farthest_node_sampling(13, initial=159)
    # print(f'Selected Nodes {' '.join([s.replace("st", '') for s in selected_nodes])}')


    # gateway_graph = np.zeros((len(gnd_gateways),len(gnd_gateways)))
    # for i in range(len(gnd_gateways)):
    #     for j in range(len(gnd_gateways)):
    #         if i > j:
    #             continue
    #         gateway_graph[i, j] = gnd_gateways[i].calculate_geo_distance(gnd_gateways[j])

    # ggs_graph = GraphNetwork([g.name for g in gnd_gateways], graph_array=gateway_graph)

    # selected_ggs = ggs_graph.farthest_node_sampling(13, initial=11)
    # print(f'Selected Gateways {' '.join([s.replace("gg", '') for s in selected_ggs])}')

    # hosts_ids = [n.id for n in selected_nodes]

    sim_total_seconds = args.sim_seconds

    # Calculate satellites orbits and ground stations positions
    propagate_orbit(sat_nodes, gnd_nodes, args.sim_seconds)

    # Get network measurements
    dataset_name = None
    
    # dataset_name = 'sandbox/space_vs_ground_note/data/sims/space_clouds_05_06_25/terrestrial/198sats_9gnd_0sts_1ggs_10dev_5400seconds_2orch_03-06-25_22-12-25'
    # dataset_name = 'sandbox/space_vs_ground_note/data/sims/space_clouds_05_06_25/terrestrial/198sats_9gnd_0sts_10ggs_10dev_5400seconds_2orch_04-06-25_06-30-14'
    # dataset_name = 'sandbox/space_vs_ground_note/data/sims/space_clouds_05_06_25/farthest-choice/198sats_9gnd_1sts_0ggs_10dev_5400seconds_2orch_04-06-25_08-08-11'
    # dataset_name = 'sandbox/space_vs_ground_note/data/sims/space_clouds_05_06_25/farthest-choice/198sats_9gnd_10sts_0ggs_10dev_5400seconds_2orch_04-06-25_16-24-16'
    tasks = []
    if dataset_name:
        tasks = get_dataset(None, data_dir=dataset_name)[0]
    # task = [t for t in tasks if t['ip'] == '10.0.0.199'][0]
    # gn199 = [g for g in gnd_nodes if g.id == 199-198][0]
    # gg208 = [g for g in gnd_nodes if g.id == 208-198-9][0]
    # st147 =  [s for s in sat_nodes if s.id == 147][0]
    # st146 =  [s for s in sat_nodes if s.id == 146][0]

    # print(((distance(gn199, st147, 13) + distance(st147, st146, 13) + distance(st146, gg208, 13)) * 1e3 / 2.998e+5 + 3 + 13)*4)


    app = QtWidgets.QApplication(sys.argv)
    plotter = PlotConstellation()
    gn_nodes = gnd_nodes+gnd_gateways if gnd_gateways and len(gnd_gateways) > 0 else gnd_nodes

    ue_nodes = []
    draw_th = Thread(target=draw_loop,
                     args=(plotter, args.sim_seconds, sat_nodes, gn_nodes, ue_nodes, sat_servers, gnd_servers, tasks, 600))
    draw_th.start()
    sys.exit(app.exec_())



