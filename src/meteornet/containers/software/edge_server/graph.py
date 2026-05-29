import pickle
import uuid
import numpy as np
from scipy.sparse import csr_array
from scipy.sparse.csgraph import dijkstra
import random

class GraphNetwork:

    def __init__(self, nodes, graph_array=None, time=0):
        if graph_array is not None:
            self.graph_array = np.array(graph_array)
            self.nodes = nodes
            self.nodes_dic = {node: i for i, node in enumerate(nodes)}
            self.nds=None
            self.graph = csr_array(self.graph_array)
        else:
            self.nodes_dic = {node.name: i for i, node in enumerate(nodes)}
            self.nodes = [n.name for n in nodes]
            self.nds =nodes
            self.make_graph(time)
            # self.links = {}

        
        # print(self.graph)

    def make_graph(self, time=0):
        self.graph_array = np.zeros((len(self.nds), len(self.nds)))
        for i, node1 in enumerate(self.nds):
            link_nodes = node1.neighbors if 'st' in node1.name else list(node1.active_links)
            for node2 in link_nodes:
                j = self.nodes_dic[node2.name]
                if self.graph_array[i,j] == 0 and self.graph_array[j, i] == 0:
                    self.graph_array[i,j] = self.distance(node1, node2, time)
        self.graph = csr_array(self.graph_array)


    def dijkstra(self, node_name, time=None):
        if time:
            self.make_graph(time)
        i_sat = self.nodes_dic[node_name]
        dist_matrix, predecessors = dijkstra(csgraph=self.graph, directed=False, indices=i_sat, return_predecessors=True)
        dist_matrix[i_sat] = np.inf
        return dist_matrix, predecessors

    def farthest_node_sampling(self, sampling_nodes, initial=0, init_t=0, end_t=91, step=15):
        selected = []
        # 1. Choose an arbitrary start node.
        # start_node = arbitrary_choice(graph.nodes)
        # start_node = random.choice(self.nodes)
        start_node = self.nodes[initial]
        selected.append(start_node)
        
        while len(selected) < sampling_nodes:
            max_distance = -1
            next_node = None
            
            # Consider each node not yet selected.
            for node in self.nodes:
                if node in selected:
                    continue
                    
                # Compute the minimum distance from 'node' to any selected node.
                dis_list = []
                for time in range(init_t, end_t, step):
                    distances, _ = self.dijkstra(node, init_t)
                    dis_list.append(distances)
                distances= np.sum(dis_list, axis=0)/len(dis_list) 
                selected_indices = np.array([self.nodes_dic[n] for n in selected])
                # min_index = selected_indices[np.argmin(distances[selected_indices])]
                min_dist = np.min(distances[selected_indices])
                # min_dist = min(dijkstra(graph, s, node) for s in selected)
                
                if min_dist > max_distance:
                    max_distance = min_dist
                    next_node = node
            
            if next_node is None:
                break  # In case we run out of candidates
            
            selected.append(next_node)
        return selected

    def distance(self, n1, n2, time):
        return np.sqrt(((n1.calculate_position(time) - n2.calculate_position(time))**2).sum())


# class Link:

    # def __init__(self, n1, n2):
    #     self.nodes = {n1, n2}

    # def distance(n1, n2):
    #     return np.sqrt(((n1.calculate_position() - n2.calculate_position())**2).sum())

    # def get_id(n1, n2):
    #     return  uuid.uuid5(uuid.NAMESPACE_URL, pickle.dumps({n1.name, n2.name})).hex
