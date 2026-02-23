#!/bin/bash
# 1. Compile (make sure it is done in build directory)
cd ~/Project_BraveHart_V2/build
make i2c_scan -j4

# 2. flash
sudo picotool load -x src/firmware/pico_i2c_scan/i2c_scan.uf2 -f

# 3. automatic monitoring
echo "Flashing successful! Opening Serial..."
sleep 1 # wait for serial port to load
