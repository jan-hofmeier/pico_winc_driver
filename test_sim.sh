#!/bin/bash
set -e
cd build_simulator
make flash
sleep 1
stty -F /dev/ttyACM0 raw
#timeout 30 head -n 3000 /dev/ttyACM0
cat -u /dev/ttyACM0
#picocom --imap lfcrlf -b 115200 /dev/ttyACM0
