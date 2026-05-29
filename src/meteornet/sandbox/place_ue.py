import numpy as np
import geopy.distance
import random
random.seed(30)

def generate_points(lat, lon, n, distance_km):
    points = []
    angle = 92.5
    for i in range(n):
        print(f'distance: {distance_km * i}')
        # angle = random_angle = random.uniform(0, 360)
        # Compute new point using geodesic distance
        new_point = geopy.distance.geodesic(kilometers=distance_km * i).destination((lat, lon), angle)
        points.append((new_point.latitude, new_point.longitude))
    
    return points

# Example usage
# lat, lon = 44.41, 8.93  # Genoa
lat, lon = 40.353076248162814, -3.796141870645394 # Madrid
# lat, lon = 38.6881374733846,-4.10838472285263
n = 9
direction = "E"
print(f'Arc is {np.pi *6370}')
distance_km = np.pi *6370 /(n-1)  # 1 km spacing

points = generate_points(lat, lon, n, distance_km)
for p in points:
    print(f'{p[0]} {p[1]} 0.0')