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
    get_computation_time
    python -m  plots.plot_database --export
    

    # Kill ONOS
    ./python_sudo.sh main.py -c
    echo 'Cleaning'
    screen -S onos -X stuff $'\003'
    kill -9 $(ps ax | grep onos | grep -F -v grep | awk '{ print $1 }')
    screen -wipe onos
}

get_computation_time () {
    c_time=`mongosh sat_net  --host localhost --quiet --eval 'let r = db.tasks.aggregate([{ $match: { status: "Done" } }, { $group: { _id: null, avgTime: { $avg: "$computation_time" } } }]).toArray(); ((r.length > 0) ? print(r[0].avgTime) : print(-1));'`
    echo "Got mean computation time of $c_time"
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

# farthest_set="105 192 99 145 52 162 58 38 142 125 32 149"
# gateway_set="24 19 17 8 26 16 14 9 4"

# hybrid_sset="1"
# hybrid_gset="12"

farthest_set="56 154 39 96 103 10 3 123 180"
gateway_set="24 19 17 8 26 16 14 9 4"

hybrid_sset="160"
hybrid_gset="12"


# list1="a b c"
# list2="1 2"

while true; do
  len1=$(echo "$hybrid_sset" | wc -w)
  len2=$(echo "$hybrid_gset" | wc -w)
  total_len=$((len1 + len2))
  echo "Length: total=$total_len"

  if (( total_len >= 10 )); then
    echo "Done! Total number of elements reached $total_len"
    break
  fi

  read -r first_farthest rest_f <<< "$farthest_set"
  read -r second_farthest rest_f2 <<< "$rest_f"

  read -r first_gateway rest_g <<< "$gateway_set"
  read -r second_gateway rest_g2 <<< "$rest_g"
  


# Advance with two space set
  h_sset="$hybrid_sset $first_farthest $second_farthest"
  run_sat_emulation "$h_sset" "$hybrid_gset" "$g_nodes"
#   get_computation_time
  c_space="$c_time"
  echo "Test Sim hg: $hybrid_gset hs: $h_sset ctime: $c_time" >> hybrid_network.log

# Advance with two ground set
  h_gset="$hybrid_gset $first_gateway $second_gateway"
  run_sat_emulation "$hybrid_sset" "$h_gset" "$g_nodes"
#   get_computation_time
  c_ground="$c_time"
  echo "Test Sim hg: $h_gset hs: $hybrid_sset ctime: $c_time" >> hybrid_network.log

# Advance with one space and one ground
  h_sset2="$hybrid_sset $first_farthest"
  h_gset2="$hybrid_gset $first_gateway"
  run_sat_emulation "$h_sset2" "$h_gset2" "$g_nodes"
  c_hybrid="$c_time"
  echo "Test Sim hg: $h_gset2 hs: $h_sset2 ctime: $c_time" >> hybrid_network.log

# if (( c_space <= c_ground && c_space > 0 ))
  if awk "BEGIN {exit !($c_space <= $c_ground && $c_space <= $c_hybrid)}"; then
    farthest_set="$rest_f2"
    hybrid_sset="$h_sset"
    echo "Choose space" >> hybrid_network.log
  elif awk "BEGIN {exit !($c_ground <= $c_space && $c_ground <= $c_hybrid)}"; then
    gateway_set="$rest_g2"
    hybrid_gset="$h_gset"
    echo "Choose ground" >> hybrid_network.log
  else
    farthest_set="$rest_f"
    gateway_set="$rest_g"
    hybrid_sset="$h_sset2"
    hybrid_gset="$h_gset2"
    echo "Choose hybrid" >> hybrid_network.log
  fi
  echo "Choose hg: $hybrid_gset hs: $hybrid_sset cpsace: $c_space cground: $c_ground chybrid: $c_hybrid"
  echo "Choose hg: $hybrid_gset hs: $hybrid_sset cpsace: $c_space cground: $c_ground chybrid: $c_hybrid" >> hybrid_network.log
done





# Ground Cloud One Server
# run_sat_emulation "" 12 "$g_nodes"


