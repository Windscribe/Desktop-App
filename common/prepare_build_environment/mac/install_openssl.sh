#!/bin/bash

echo "Installing openssl into $HOME/LibsWindscribe/openssl"

rm -rf $HOME/"openssl_temp"
rm -rf $HOME/"LibsWindscribe/openssl"

mkdir $HOME/"openssl_temp"
curl https://www.openssl.org/source/openssl-1.1.1d.tar.gz -o $HOME/"openssl_temp/openssl-1.1.1d.tar.gz" -k -L
tar -zxvf $HOME/"openssl_temp/openssl-1.1.1d.tar.gz" -C $HOME/"openssl_temp"

pushd $HOME/openssl_temp/openssl-1.1.1d

export CC="cc -mmacosx-version-min=10.11"

./config --prefix=$HOME/"LibsWindscribe/openssl" --shared no-asm

make
make install

popd
rm -rf $HOME/"openssl_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/openssl.zip "$HOME/LibsWindscribe/openssl"
fi