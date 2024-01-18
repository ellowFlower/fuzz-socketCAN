#!/bin/bash
# This script installs the Linux MCU code to /usr/local/bin/

if [ "$EUID" -ne 0 ]; then
    echo "This script must be run as root"
    exit -1
fi
set -e

# Setting build output directory
if [ -z "${1}" ]; then
    out='out'
else
    out=${1}
fi

# Install new micro-controller code
echo "Installing micro-controller code to /usr/local/bin/"
rm -f /usr/local/bin/klipper_mcu
cp ${out}/klipper.elf ${out}/klipper
sync
