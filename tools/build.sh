#! /bin/sh

set -e
set -x

mkdir build || true
cd build

#cmake ..
#make

cmake -G "Ninja" ..
TERM=dumb # disable line-clearing in ninja
ninja #-d explain # -v
