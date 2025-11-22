#!/bin/bash
set -e
mkdir -p build
cd build
cmake .. -DBUILD_MODE=COMBINED
make pico_winc_combined
picotool load -F pico_winc_combined.uf2
picotool reboot