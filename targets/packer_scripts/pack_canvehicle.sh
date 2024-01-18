#!/bin/bash
set -e
echo $PWD
source ./config.sh

SHAREDIR="../packed_targets/nyx_canvehicle/"

if [ "$1" = "docker" ]; then
  DEFAULT_CONFIG="--path_to_default_config ../default_config_kernel.ron"
else 
  DEFAULT_CONFIG=""
fi

python3 $PACKER ../setup_scripts/build/canvehicle/canvehicle/canvehicle $SHAREDIR m64 \
--afl_mode \
--purge \
--nyx_net \
--nyx_net_can \
-spec ../specs/canvehicle/ \
--setup_folder ../extra_folders/canlinux && \
python3 $CONFIG_GEN $SHAREDIR Kernel -s ../specs/canvehicle/ $DEFAULT_CONFIG
