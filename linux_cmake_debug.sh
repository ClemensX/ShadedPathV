#!/usr/bin/bash
#
# This script is used to build with cmake on Linux
#-DCMAKE_MAKE_PROGRAM=/usr/bin/make \
cmake -S . -B ./build -DCMAKE_C_COMPILER=/usr/bin/gcc \
-DCMAKE_CXX_COMPILER=/usr/bin/g++ \
-G Ninja \
-DCMAKE_BUILD_TYPE=Debug \
-DCMAKE_TOOLCHAIN_FILE=~/cpp/vcpkg/scripts/buildsystems/vcpkg.cmake