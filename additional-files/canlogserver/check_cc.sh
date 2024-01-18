#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
# check_cc.sh - Helper to test userspace compilation support
# Copyright (c) 2015 Andrew Lutomirski
# Commit 5ed3b4ded6cf3e4de6fc8c8739b84231b0285b0e

CC="$1"
TESTPROG="$2"
shift 2

if [ -n "$CC" ] && $CC -o /dev/null "$TESTPROG" -O0 "$@"; then
    echo 1
else
    echo 0
fi

exit 0