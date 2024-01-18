#!/bin/bash
set -e
echo $PWD
source ./config.sh

SHAREDIR="../packed_targets/nyx_customsocket/"

if [ "$1" = "docker" ]; then
  DEFAULT_CONFIG="--path_to_default_config ../default_config_kernel.ron"
else 
  DEFAULT_CONFIG=""
fi

python3 $PACKER ../setup_scripts/build/customsocket/customsocket/customsocket $SHAREDIR m64 \
--afl_mode \
--purge \
--nyx_net \
--nyx_net_port 5353 \
--nyx_net_can \
-spec ../specs/customsocket/ \
--setup_folder ../extra_folders/customsocket && \
python3 $CONFIG_GEN $SHAREDIR Kernel -s ../specs/customsocket/ $DEFAULT_CONFIG