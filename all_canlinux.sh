#!/bin/bash

./setup.sh && \
cd targets/setup_scripts/canlinux && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_canlinux.sh && \
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_canlinux/ -p aggressive -t 5