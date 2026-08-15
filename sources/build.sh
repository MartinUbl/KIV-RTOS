#!/bin/bash

mkdir -p build >/dev/null 2>&1
cd build

EXPANSION_BOARD=NONE
if [ "$1" == "KIV-DPP-01" ]; then
    EXPANSION_BOARD=KIVDPP01
elif [ "$1" == "KIV-DPP-02" ]; then
    EXPANSION_BOARD=KIVDPP02
fi

cmake -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="../misc/cmake/toolchain-arm-none-eabi-rpi0.cmake" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DUSE_EXPANSION_BOARD=$EXPANSION_BOARD ..

cmake --build . --parallel
#make
#make VERBOSE=1
