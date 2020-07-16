#!/bin/bash

echo "Installing lzo into $HOME/LibsWindscribe/lzo"

rm -rf $HOME/"lzo_temp"
rm -rf $HOME/"LibsWindscribe/lzo"

mkdir $HOME/"lzo_temp"
curl http://www.oberhumer.com/opensource/lzo/download/lzo-2.10.tar.gz -o $HOME/"lzo_temp/lzo-2.10.tar.gz" -k -L
tar -zxvf $HOME/"lzo_temp/lzo-2.10.tar.gz" -C $HOME/"lzo_temp"

pushd $HOME/"lzo_temp/lzo-2.10"
export CC="cc -mmacosx-version-min=10.11"
./configure --prefix=$HOME/"LibsWindscribe/lzo"
make
make install

popd
rm -rf $HOME/"lzo_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/lzo.zip "$HOME/LibsWindscribe/lzo"
fi