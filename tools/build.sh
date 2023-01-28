#! /bin/sh

set -e
set -x

mkdir build || true

# gnumake
#cmake ..
#make

# ninja
cmake -G "Ninja" -B build
TERM=dumb # disable line-clearing in ninja
ninjaFlags=()
#ninjaFlags+=(-v) # verbose: show commands
#ninjaFlags+=(-d explain) # debug
ninjaFlags+=(-j1) # single threaded
ninjaFlags+=(-C build)
ninja "${ninjaFlags[@]}"
