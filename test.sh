#!/bin/bash
set -e
./buildflash.sh
sleep 2
timeout 30 head -n 100 /dev/ttyACM0
