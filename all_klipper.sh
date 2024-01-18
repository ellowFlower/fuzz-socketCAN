./setup.sh && \
cd targets/setup_scripts/klipper && \
sh build.sh no_asan && \
cd ../../packer_scripts && \
./pack_klipper.sh && \
cd ../../fuzzer/rust_fuzzer && \
cargo run --release --verbose --       -s ../../targets/packed_targets/nyx_klipper/