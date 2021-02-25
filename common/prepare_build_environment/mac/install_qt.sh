#!/bin/bash

echo "Installing Qt into $HOME/Qt"

rm -rf $HOME/"qt_temp"
rm -rf $HOME/"Qt"

mkdir $HOME/"qt_temp"
curl https://download.qt.io/archive/qt/5.15/5.15.2/single/qt-everywhere-src-5.15.2.tar.xz -o $HOME/"qt_temp/qt-everywhere-src-5.15.2.tar.xz" -k -L

tar -zxvf $HOME/"qt_temp/qt-everywhere-src-5.15.2.tar.xz" -C $HOME/"qt_temp"

pushd $HOME/"qt_temp/qt-everywhere-src-5.15.2"

export OPENSSL_LIBS="-L$HOME/LibsWindscribe/openssl/lib -lssl -lcrypto"
./configure -opensource -confirm-license -openssl-linked "-I$HOME/LibsWindscribe/openssl/include" -release -nomake examples -prefix $HOME/"Qt" -skip connectivity -skip qtwebengine -skip qtwebchannel -skip multimedia -skip qtwebview

make -j8
make install

popd
rm -rf $HOME/"qt_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/Qt.zip "$HOME/Qt"
fi
