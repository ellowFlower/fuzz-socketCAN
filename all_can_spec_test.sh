#!/bin/bash

./setup.sh && \
cd targets/setup_scripts/canspectest && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_canspectest.sh && \
gnome-terminal -- bash -c "candump -ta any; exec bash" && \
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_canspectest/