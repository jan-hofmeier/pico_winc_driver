#!/bin/bash
set -e
./buildflash.sh
sleep 2
stty -F /dev/ttyACM0 raw
timeout 30 head -n 100 /dev/ttyACM0
