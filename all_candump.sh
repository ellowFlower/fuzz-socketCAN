#!/bin/bash

./setup.sh && \
cd targets/setup_scripts/candump && \
sh build.sh no_asan
cd ../../packer_scripts && \
./pack_candump.sh
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_candump/
