#!/bin/sh

BUILD_DIR=build

mkdir -p $BUILD_DIR

cd $BUILD_DIR || exit
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
make

./openglfun
