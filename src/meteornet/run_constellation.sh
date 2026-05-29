#!/bin/bash

if grep -q "Arch Linux" /etc/os-release; then
    echo "Running for Arch Linux..."
    sudo sudo systemctl start ovs-vswitchd
    sudo sudo systemctl start ovsdb-server
    source onos.bashrc
    mongo_cmd=mongodb
    export bazel_cmd=bazel
elif grep -q "Ubuntu" /etc/os-release; then
    echo "Running for Ubuntu..."
    source onos.bashrc
    mongo_cmd=mongod
    export bazel_cmd=bazel-3.7.2
else
    echo "Unsupported OS."
    return -1
fi

start_onos () {
    echo 'Starting ONOS'
    echo $bazel_cmd $ONOS_ROOT
    screen -S onos -dm  bash -c 'cd $ONOS_ROOT && $bazel_cmd run onos-local -- clean'
    echo 'Sleeping 60 second to wait for onos'
    sleep 60
}

clean_network () {
    echo 'Cleaning'
    ./python_sudo.sh main.py -c
    screen -S onos -X stuff $'\003'
    kill -9 $(ps ax | grep onos | grep -F -v grep | awk '{ print $1 }')
    screen -wipe
}

run_sat_emulation () {
    # Start ONOS
    start_onos

    echo 'Start Constellation Emulation'
    ./python_sudo.sh main.py --sats  $sats --sat_servers $1 --gnd_servers $2 -s $sim_time -x 1  --step $step --gnds $3 --edge_orchestration 2 --devices $devices --seed $seed --tle_path $tle_path
    # ./python_sudo.sh main.py --sats  500 --hosts 1 2 3 4 5 6 7 8 9 10 -s 180 -x 1 --cloud_mode 0  --step 30 --gnds $gnd --edge_orchestration 2 --devices 20 --seed $seed --sflow --tle_path tles/starlink
    echo 'Export Database'
    python -m  plots.plot_database --export

    clean_network
}

# sudo fuser -k 8008/tcp
clean_network
sudo systemctl start $mongo_cmd


seed=10
sim_time=5400
tle_path=tles/starlink_198
devices=10
sats=198
step=60
g_nodes="1 2 3 4 5 6 7 8 9"
s_servers="1 105 192"
g_servers="12"

# orchestration 2 -> All servers on
# orchestration 4 -> Reinforcement learning controller on servers
orch=2

run_sat_emulation "s_servers" "g_servers" "$g_nodes"