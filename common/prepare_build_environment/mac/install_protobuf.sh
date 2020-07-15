#!/bin/bash

echo "Installing protobuf into $HOME/LibsWindscribe/protobuf"

rm -rf $HOME/"protobuf_temp"
rm -rf $HOME/"LibsWindscribe/protobuf"

# must have autotools installed before running this script (autoconf, automake and libtool)

# get and extract
mkdir $HOME/"protobuf_temp"
curl https://github.com/protocolbuffers/protobuf/archive/v3.8.0.tar.gz -o $HOME/"protobuf_temp/protobuf-3.8.0.tar.gz" -k -L
tar -zxvf $HOME/"protobuf_temp/protobuf-3.8.0.tar.gz" -C $HOME/"protobuf_temp"

pushd $HOME/"protobuf_temp/protobuf-3.8.0"

export CC="cc -mmacosx-version-min=10.11"

./autogen.sh
./configure --prefix=$HOME/LibsWindscribe/protobuf
make
make check
sudo make install

popd
rm -rf $HOME/"protobuf_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/protobuf.zip "$HOME/LibsWindscribe/protobuf"
fi
