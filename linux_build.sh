#!/usr/bin/bash
#
# This script is used to build with cmake on Linux
cmake --build ./build -- -j$(nproc)