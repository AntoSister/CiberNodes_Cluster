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

    ./python_sudo.sh main.py --sats  $sats --sat_servers $1 --gnd_servers $2 -s $sim_time -x 1  --step $step --gnds $3 --edge_orchestration $orch --devices $devices --seed $seed --tle_path $tle_path
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
sim_time=5400
tle_path=tles/starlink_198
gnd_file=Coordinates_uniform2.txt
devices=10
sats=198
g_nodes="1 2 3 4 5 6 7 8 9"
step=60

# orchestration 2 -> All servers on
# orchestration 4 -> Reinforcement learning controller on servers
orch=2

# Ground Cloud One Server
# run_sat_emulation "" 12 "$g_nodes"

# Ground Cloud 2 Server
# run_sat_emulation "" "12 24" "$g_nodes"

# Ground Cloud 4 Server
# run_sat_emulation "" "12 24 19 17" "$g_nodes"

# Ground Cloud 6 Server
# run_sat_emulation "" "12 24 19 17 8 26" "$g_nodes"

# Ground Cloud 8 Server
# run_sat_emulation "" "12 24 19 17 8 26 16 14" "$g_nodes"

# Ground Cloud 10 Server
# run_sat_emulation "" "12 24 19 17 8 26 16 14 9 4" "$g_nodes"

# Ground Cloud 13 Server
# run_sat_emulation "" "12 24 19 17 8 26 16 14 9 4 7 6 28" "$g_nodes"

# Space Cloud One Server Sat
# run_sat_emulation 160 "" "$g_nodes"

# Space Cloud Farthest Point 2
# run_sat_emulation "160 56" "" "$g_nodes"

# Space Cloud Farthest Point 4
# run_sat_emulation "160 56 154 39" "" "$g_nodes"

# Space Cloud Farthest Choice 6
# run_sat_emulation "160 56 154 39 96 103" "" "$g_nodes"

# Space Cloud Farthest Choice 8
# run_sat_emulation "160 56 154 39 96 103 10 3" "" "$g_nodes"

# Space Cloud Farthest Choice 10
# run_sat_emulation "160 56 154 39 96 103 10 3 123 180" "" "$g_nodes"

# Space Cloud Farthest Choice 13
# run_sat_emulation "160 56 154 39 96 103 10 3 123 180 99 53 7 "" "$g_nodes"

# Hybrid Cloud 2
run_sat_emulation "160" "12" "$g_nodes"

# Hybrid Cloud 4
# run_sat_emulation "160 56 154" "12" "$g_nodes"

# Hybrid Cloud 6
# run_sat_emulation "160 56 154 39" "12 24" "$g_nodes"

# Hybrid Cloud 8
# run_sat_emulation "160 56 154 39 96 103" "12 24" "$g_nodes"

# Hybrid Cloud 10
# run_sat_emulation "160 56 154 39 96 103 10 3" "12 24" "$g_nodes"

# Space Cloud Cross Plane 2
# run_sat_emulation "6 160" "" "$g_nodes"

# Space Cloud Cross Plane 4
# run_sat_emulation "6 94 105 160" "" "$g_nodes"

# Space Cloud Cross Plane 6
# run_sat_emulation "6 50 94 105 138 160" "" "$g_nodes"

# Space Cloud Cross Plane 8
# run_sat_emulation "6 28 50 94 105 138 160 193" "" "$g_nodes"

# Space Cloud Cross Plane 10
# run_sat_emulation "6 28 50 72 94 105 116 138 160 193" "" "$g_nodes"

# Space Cloud Along Orbit 2
# run_sat_emulation "159 160" "" "$g_nodes"

# Space Cloud Along Orbit 4
# run_sat_emulation "159 160 161 162" "" "$g_nodes"

# Space Cloud Along Orbit 6
# run_sat_emulation "158 159 160 161 162 163" "" "$g_nodes"

# Space Cloud Along Orbit 8
# run_sat_emulation "157 158 159 160 161 162 163 164" "" "$g_nodes"

# Space Cloud Along Orbit 10
# run_sat_emulation "156 157 158 159 160 161 162 163 164 165" "" "$g_nodes"

