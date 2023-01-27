#! /bin/sh

set -e

mkdir build
cd build

#cmake ..
#make

#TERM=dumb # disable line-clearing in ninja
cmake -G "Ninja" ..
ninja
