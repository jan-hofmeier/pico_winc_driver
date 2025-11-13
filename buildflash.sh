#!/bin/bash
set -e
mkdir -p build
cd build
make
picotool load -F pico_winc_driver.uf2
picotool reboot
