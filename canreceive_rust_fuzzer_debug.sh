#!/bin/bash

cd ./fuzzer/rust_fuzzer_debug && \
cargo run --release -- \
      -s ../../targets/packed_targets/nyx_canlinux/ \
      -d /tmp/workdir/corpus/crash/ \
      -t /tmp/workdir/corpus/crash_reproducible/