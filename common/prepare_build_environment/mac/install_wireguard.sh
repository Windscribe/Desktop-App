#!/bin/bash

echo "Installing WireGuard"

rm -rf $HOME/"wireguard_temp"
rm -rf $HOME/"LibsWindscribe/wireguard"

# Install tools first:
# brew install swiftlint go

mkdir $HOME/"wireguard_temp"

WGDIR=wireguard-go-0.0.20200320
curl https://git.zx2c4.com/wireguard-go/snapshot/$WGDIR.tar.xz -o $HOME/"wireguard_temp/wireguard.tar.xz" -k -L
tar -zxvf $HOME/"wireguard_temp/wireguard.tar.xz" -C $HOME/"wireguard_temp"

pushd $HOME/"wireguard_temp/"$WGDIR
export DESTDIR="$HOME/LibsWindscribe/"
export BINDIR="wireguard"
mkdir "$DESTDIR/$BINDIR"
make install
popd

rm -rf $HOME/"wireguard_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/wireguard.zip "$HOME/LibsWindscribe/wireguard"
fi
