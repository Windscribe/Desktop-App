#!/bin/bash

echo "Installing cares into $HOME/LibsWindscribe/cares"

rm -rf $HOME/"cares_temp"
rm -rf $HOME/"LibsWindscribe/cares"

mkdir $HOME/"cares_temp"
curl https://c-ares.haxx.se/download/c-ares-1.17.1.tar.gz -o $HOME/"cares_temp/c-ares-1.17.1.tar.gz" -k -L

tar -zxvf $HOME/"cares_temp/c-ares-1.17.1.tar.gz" -C $HOME/"cares_temp"

pushd $HOME/"cares_temp/c-ares-1.17.1"

export CC="cc -mmacosx-version-min=10.11"
./configure --prefix=$HOME/"LibsWindscribe/cares"
make
make install

popd
rm -rf $HOME/"cares_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/cares.zip "$HOME/LibsWindscribe/cares"
fi