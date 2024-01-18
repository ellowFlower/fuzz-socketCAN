./setup.sh && \
cd targets/setup_scripts/canvehicle && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_canvehicle.sh && \
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_canvehicle/ -t 3