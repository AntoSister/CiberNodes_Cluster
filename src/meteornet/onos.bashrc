
ONOS_ROOT=$(cd ../onos && pwd)
#ONOS_ROOT="/home/kaminari/Development/scnl/software/sat-edge-computation/onos"
source $ONOS_ROOT/tools/dev/bash_profile
export ONOS_APPS="$ONOS_APPS,fwd"
export JAVA_OPTS="-Dkaraf.log.console=ERROR"
export FLOW_TIMEOUT_DEFAULT=1
export PROBE_RATE_DEFAULT=500