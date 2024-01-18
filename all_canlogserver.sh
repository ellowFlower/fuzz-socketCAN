#!/bin/bash

./setup.sh && \
cd targets/setup_scripts/canlogserver && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_canlogserver.sh && \
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_canlogserver/ -p aggressive -t 3