#!/bin/bash

echo "Installing openvpn_2_5_0 into $HOME/LibsWindscribe/openvpn_2_5_0"

rm -rf $HOME/"openvpn_temp"
rm -rf $HOME/"LibsWindscribe/openvpn_2_5_0"

mkdir $HOME/"openvpn_temp"
curl https://swupdate.openvpn.org/community/releases/openvpn-2.5.0.tar.gz -o $HOME/"openvpn_temp/openvpn-2.5.0.tar.gz" -k -L
tar -zxvf $HOME/"openvpn_temp/openvpn-2.5.0.tar.gz" -C $HOME/"openvpn_temp"

pushd $HOME/"openvpn_temp/openvpn-2.5.0"
export CFLAGS="-I$HOME/LibsWindscribe/openssl/include -I$HOME/LibsWindscribe/lzo/include"
export CPPFLAGS="-I$HOME/LibsWindscribe/openssl/include -I$HOME/LibsWindscribe/lzo/include"
export LDFLAGS="-L$HOME/LibsWindscribe/openssl/lib -L$HOME/LibsWindscribe/lzo/lib"

export CC="cc -mmacosx-version-min=10.11"
./configure --with-crypto-library=openssl --prefix=$HOME/"LibsWindscribe/openvpn_2_5_0"
make
make install

popd
rm -rf $HOME/"openvpn_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/openvpn_2_5_0.zip "$HOME/LibsWindscribe/openvpn_2_5_0"
fi
