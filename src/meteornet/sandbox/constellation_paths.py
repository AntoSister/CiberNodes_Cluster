import requests


ONOS_URL = "http://localhost:8181"
AUTH = ("onos", "rocks")

def get_host(ip):
    url = f"{ONOS_URL}/onos/v1/hosts"
    try:
        response = requests.get(url, auth=AUTH)
        response.raise_for_status()
        hosts = response.json().get("hosts", [])
        for host in hosts:
            # The 'ips' field contains a list of IPs associated with the host.
            if ip in host.get("ipAddresses", []):
                return host.get("locations")[0].get('elementId')
    except requests.exceptions.RequestException as e:
        print(f"Error querying hosts: {e}")
    return None

def get_path(src_host_id, dst_host_id):
    url = f"{ONOS_URL}/onos/v1/paths/{src_host_id}/{dst_host_id}"
    try:
        response = requests.get(url, auth=AUTH)
        response.raise_for_status()
        paths = response.json().get("paths", [])
        if paths:
            return paths[0]
    except requests.exceptions.RequestException as e:
        print(f"Error querying paths: {e}")
    return None

def get_devices_path(path):
    url = f"{ONOS_URL}/onos/v1/devices"
    try:
        response = requests.get(url, auth=AUTH)
        response.raise_for_status()
        devices = response.json().get("devices", [])
        if devices:
            devices_dict = {d['id'] : d['annotations']['datapathDescription']  for d in devices}
            path_names = []
            for node in path['links']:
                id = node['src']['device']
                path_names.append(devices_dict[id])
                if node ==  path['links'][-1]:
                    id = node['dst']['device']
                    path_names.append(devices_dict[id])

            return path_names
    except requests.exceptions.RequestException as e:
        print(f"Error in devices: {e}")
    #http://localhost:8181/onos/v1/devices
    return None

def main():
    src_ip = "10.0.0.31" 
    dst_ip = "10.0.0.1"

    # Get the corresponding host IDs for the IPs
    src_host_id = get_host(src_ip)
    dst_host_id = get_host(dst_ip)

    if not src_host_id or not dst_host_id:
        print("Error: One or both hosts were not found in ONOS.")
        return

    # Get the network path between the two hosts
    path = get_path(src_host_id, dst_host_id)
    if path:
        print("Path found:")
        print(get_devices_path(path))
    else:
        print("No path found between the specified hosts.")

if __name__ == "__main__":
    main()