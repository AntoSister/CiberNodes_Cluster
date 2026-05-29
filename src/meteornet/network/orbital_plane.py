import numpy as np
import math


class OrbitalPlane:
    def __init__(self, sat_node):
        # sat = sat_node.satrec()
        self.normal = sat_node.calculate_plane_normal()
        self.inclination = sat_node.epoch.inclo
        self.nodeo = sat_node.epoch.nodeo
        self.argpo = sat_node.epoch.argpo
        self.sats = []
        self.add_if_in_plane(sat_node)

    def get_nearest_sat(self, sat, th_dist=3000):
        distances = [np.linalg.norm(sat.init_coords - s.init_coords) for s in self.sats]
        for ind in np.argsort(distances):
            if distances[ind] < th_dist:
                return self.sats[ind]
        return None

    def get_nearest_planes(self, orbital_planes, thr_angle=np.pi/3, max_planes=2):
        filtered_planes = [pl for pl in orbital_planes if pl is not self]
        # Calculate angle between normals in planes
        reference_angles = [np.arccos(np.dot(pl.normal, self.normal)) for pl in filtered_planes]
        plane_neighbors = []
        nearest_angle = thr_angle
        for ind in np.argsort(reference_angles):
            if len(plane_neighbors) < max_planes and reference_angles[ind] <= thr_angle:
                if len(plane_neighbors) == 0:
                    nearest_angle = reference_angles[ind]
                    plane_neighbors.append(filtered_planes[ind])
                else:
                    # Check if the difference with new plane is near to nearest plane
                    if abs(reference_angles[ind] - nearest_angle) < 0.03:
                        plane_neighbors.append(filtered_planes[ind])
            else:
                break
        return plane_neighbors

    def get_neighbors(self, sat,  thr_angle=np.pi/3, max_neighbors=2):
        curr_anomaly = sat.epoch.mo
        filt_sats = [s for s in self.sats if s != sat]
        mean_anomalies = np.array([s.epoch.mo for s in filt_sats])
        diff_angles = np.array([abs(math.remainder(ang, 2 * math.pi)) for ang in (mean_anomalies - curr_anomaly)])
        sat_neighbors = []
        for ind in np.argsort(diff_angles):
            if len(sat_neighbors) < max_neighbors and abs(diff_angles[ind]) <= thr_angle:
                sat_neighbors.append(filt_sats[ind])
            if len(sat_neighbors) >= max_neighbors:
                break
        return sat_neighbors

    def add_if_in_plane(self, sat):
        if np.linalg.norm(sat.calculate_plane_normal() - self.normal) < 1E-2:
            if sat not in self.sats:
                self.sats.append(sat)
            if sat.orbital_plane != self:
                sat.set_orbital_plane(self)
            return True
        return False

    @staticmethod
    def group_by_orbital_plane(sat_nodes):
        orbital_groups = []
        for node in sat_nodes:
            if len(orbital_groups) == 0:
                orbital_groups.append(OrbitalPlane(node))
                continue
            else:
                add_to_set = False
                for orb in orbital_groups:
                    add_to_set = orb.add_if_in_plane(node)
                    if add_to_set:
                        break
                if not add_to_set:
                    orbital_groups.append(OrbitalPlane(node))
        len_i = np.argsort([len(o.sats) for o in orbital_groups])[::-1]
        return [orbital_groups[i] for i in len_i]


