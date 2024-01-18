cd setup_scripts/canlinux && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_canlinux.sh
