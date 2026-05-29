#!/usr/bin/env bash
echo "Downloading SUCHAI Flight Software"
[ ! -d suchai-flight-software ] && git clone -b feature/framework-sim https://gitlab.com/spel-uchile/suchai-flight-software.git
