#!/bin/sh

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build