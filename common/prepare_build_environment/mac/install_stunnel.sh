#!/bin/bash

echo "Installing stunnel into $HOME/LibsWindscribe/stunnel"

rm -rf $HOME/"stunnel_temp"
rm -rf $HOME/"LibsWindscribe/stunnel"

mkdir $HOME/"stunnel_temp"
curl https://www.stunnel.org/downloads/stunnel-5.58.tar.gz -o $HOME/"stunnel_temp/stunnel-5.58.tar.gz" -k -L
tar -zxvf $HOME/"stunnel_temp/stunnel-5.58.tar.gz" -C $HOME/"stunnel_temp"

pushd $HOME/"stunnel_temp/stunnel-5.58"
export CC="cc -mmacosx-version-min=10.11"
./configure --prefix=$HOME/"LibsWindscribe/stunnel" --prefix=$HOME/"LibsWindscribe/stunnel" --with-ssl=$HOME/"LibsWindscribe/openssl"
make
make install

popd
rm -rf $HOME/"stunnel_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/stunnel.zip "$HOME/LibsWindscribe/stunnel"
fi