#!/bin/bash

./setup.sh && \
cd targets/setup_scripts/customsocket && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_customsocket.sh && \
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_customsocket/