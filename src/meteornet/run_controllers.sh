#!/bin/bash

if grep -q "Arch Linux" /etc/os-release; then
    echo "Running for Arch Linux..."
    sudo systemctl start ovs-vswitchd
    sudo systemctl start ovsdb-server
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

# $1 is cloud mode: 0-> Space, 1-> Ground
# $2 is a list of hosts
run_sat_emulation () {
    sudo systemctl restart ovs-vswitchd
    sudo systemctl restart ovsdb-server
    ./python_sudo.sh main.py -c
    # Start ONOS
    echo 'Starting ONOS'
    screen -S onos -dm  bash -c 'cd $ONOS_ROOT && $bazel_cmd run onos-local -- clean'
    echo 'Sleeping 60 second to wait for onos'
    sleep 60

    ./python_sudo.sh main.py --sats  $sats --sat_servers $sat_servers --gnd_servers "" -s $sim_time -x 1  --step $step --gnds $g_nodes --edge_orchestration $2 --devices $1 --seed $seed --tle_path $tle_path
    python -m  plots.plot_database --export

    # Kill ONOS
    ./python_sudo.sh main.py -c
    echo 'Cleaning'
    screen -S onos -X stuff $'\003'
    kill -9 $(ps ax | grep onos | grep -F -v grep | awk '{ print $1 }')
    screen -wipe onos
}

sudo fuser -k 8008/tcp
sudo systemctl start $mongo_cmd
# ./python_sudo.sh main.py -c

seed=10
sim_time=1200
tle_path=tles/starlink_198
gnd_file=Coordinates_uniform.txt
# devices=10
sats=198
sat_servers="1 12 23 34 45 56 67 78 89 100 111 122 133 144 155 166 177 188"
g_nodes="1 2 3 5 7 9"
step=60
orchs="4 7"
devs="20 40 60 80 100 120"


# orchestration 2 -> All servers on
#               4 -> Fuzzy 
#               6 -> Reinforcement learning controller on servers
#               7 -> Random

for orch in $orchs; do
    for dev in $devs; do
        run_sat_emulation "$dev" "$orch"
    done
done