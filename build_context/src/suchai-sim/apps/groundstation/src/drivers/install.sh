#!/bin/sh
if  [ -n "$1" ] && [ $1 = "--ssh" ]; then
    SSH_ACTIVE="TRUE"
fi

echo "Downloading BUS drivers..."
if [ ! -d "suchai-drivers-bus" ]; then
    if [ -z "$SSH_ACTIVE" ]; then
        git clone https://gitlab.com/spel-uchile/suchai-drivers-bus suchai-drivers-bus
    else
        git clone git@gitlab.com:spel-uchile/suchai-drivers-bus.git suchai-drivers-bus
    fi
else
    cd suchai-drivers-bus
    git pull
    cd -
fi
