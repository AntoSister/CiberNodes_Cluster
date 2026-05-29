

from plots.plot_database import get_dataset
import os
import numpy as np
import math
import matplotlib.pyplot as plt 



def calculate_geo_distance(lat_lon1, lat_lon2):
    R = 6373.0
    lat1 = math.radians(lat_lon1[0])
    lon1 = math.radians(lat_lon1[1])
    lat2 = math.radians(lat_lon2[0])
    lon2 = math.radians(lat_lon2[1])


    dlon = lon2 - lon1
    dlat = lat2 - lat1

    a = math.sin(dlat / 2)**2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2)**2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    distance = R * c
    return distance

def process_data(gnd_dir, ground_cloud=True, dis_map={}):
       # Process ground data
    gnd_datasets = os.listdir(gnd_dir)
    distances = []
    net_delays = []

    ret_map = {} if ground_cloud else dis_map

    for dataset in gnd_datasets:
        gnd_data = get_dataset(None, gnd_dir + dataset)
        print('dataset', dataset)

        tasks = [t for t in gnd_data[0] if t['status']== 'Done']
        nodes = gnd_data[2]
        # servers_hist = gnd_data[3]

        get_index = lambda n, l: l[[i for i, v  in enumerate(l) if  n in v['name'] ][0]]

        gnd_node = get_index('gn', nodes)

        if ground_cloud:
            gnd_gateway = get_index('gg101', nodes)
            geo_distance = calculate_geo_distance((gnd_node['latitude'], gnd_node['longitude']),
                                                (gnd_gateway['latitude'], gnd_gateway['longitude']) )
            print('geo distance',geo_distance, gnd_node['latitude'], gnd_node['longitude'])
            ret_map.update({str(gnd_node['latitude'])+str(gnd_node['longitude']) : geo_distance})
        else:
            geo_distance = dis_map[str(gnd_node['latitude'])+str(gnd_node['longitude'])]

        if gnd_node['latitude'] == 64.0166:
            print('remove: ', dataset)

        
        distances.append(geo_distance)

        
        # task_items, servers_hist_all, nodes, servers_hist_mec_on
        network_times = [t['computation_time'] for t in tasks]
        mean_time = np.mean(network_times) - 0.25
        # print('mean network time', mean_time)
        net_delays.append(mean_time)

    return distances, net_delays, ret_map


if __name__ == '__main__':

    dataset_name = '100sats_1gnd_100dev_1200seconds_2orch_08-10-24_13-47-16'
    # space_dir = 'data/sims/space_vs_ground/space/'
    

    gnd_dir  = 'data/sims/space_vs_ground/ground/5ms/'
    gnd_distances, gnd_net, dis_map = process_data(gnd_dir)
 

    # Process space 1 server
    space_dir_1 = 'data/sims/space_vs_ground/sats/1s/'
    space_distances_1, space_net_1, _ = process_data(space_dir_1, False, dis_map)

    # Process space 10 server inplane
    space_dir_10 = 'data/sims/space_vs_ground/sats/10s_inplane/'
    space_distances_10, space_net_10, _ = process_data(space_dir_10, False, dis_map)

    # Process space 10 server ecuatorial
    space_dir_10_ec = 'data/sims/space_vs_ground/sats/10s_ecuatorial/'
    space_distances_10_ec, space_net_10_ec, _ = process_data(space_dir_10_ec, False, dis_map)
    



    fig, ax = plt.subplots()
    ax.scatter(gnd_distances, gnd_net, label='ground')
    ax.scatter(space_distances_1, space_net_1, label='space 1s')
    ax.scatter(space_distances_10, space_net_10, label='space 10s inplane')
    ax.scatter(space_distances_10_ec, space_net_10_ec, label='space 10s ecuatioral')
    ax.set_ylabel('Network Delay [ms]')
    ax.set_xlabel('Distance [m]')
    plt.legend()
    plt.show()
        
        # print(gnd_data
