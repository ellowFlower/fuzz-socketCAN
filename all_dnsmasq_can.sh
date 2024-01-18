#!/bin/bash

./setup.sh && \
cd targets/setup_scripts/dnsmasqcan && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_dnsmasqcan.sh
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_dnsmasqcan/