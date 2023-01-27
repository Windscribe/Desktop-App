#! /bin/sh

set -e
set -x

mkdir build || true
cd build

# gnumake
#cmake ..
#make

# ninja
cmake -G "Ninja" ..
TERM=dumb # disable line-clearing in ninja
ninjaFlags=()
#ninjaFlags+=(-v) # verbose: show commands
#ninjaFlags+=(-d explain) # debug
ninjaFlags+=(-j1) # single threaded
ninja "${ninjaFlags[@]}"
