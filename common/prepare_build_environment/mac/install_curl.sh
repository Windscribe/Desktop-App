#!/bin/bash

echo "Installing curl into $HOME/LibsWindscribe/curl"

rm -rf $HOME/"curl_temp"
rm -rf $HOME/"LibsWindscribe/curl"

mkdir $HOME/"curl_temp"
curl https://curl.haxx.se/download/curl-7.67.0.tar.gz -o $HOME/"curl_temp/curl-7.67.0.tar.gz" -k -L

tar -zxvf $HOME/"curl_temp/curl-7.67.0.tar.gz" -C $HOME/"curl_temp"

pushd $HOME/"curl_temp/curl-7.67.0"
export CC="cc -mmacosx-version-min=10.11"
./configure --prefix=$HOME/"LibsWindscribe/curl" --with-ssl=$HOME/"LibsWindscribe/openssl"
make
make install

popd
rm -rf $HOME/"curl_temp"


# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/curl.zip "$HOME/LibsWindscribe/curl"
fi

