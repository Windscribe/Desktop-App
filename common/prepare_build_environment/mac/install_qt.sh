#!/bin/bash

echo "Installing Qt into $HOME/Qt"

rm -rf $HOME/"qt_temp"
rm -rf $HOME/"Qt"

mkdir $HOME/"qt_temp"
curl https://download.qt.io/archive/qt/5.12/5.12.7/single/qt-everywhere-src-5.12.7.tar.xz -o $HOME/"qt_temp/qt-everywhere-src-5.12.7.tar.xz" -k -L

tar -zxvf $HOME/"qt_temp/qt-everywhere-src-5.12.7.tar.xz" -C $HOME/"qt_temp"

#cp qt_changed_files/qnsview.h $HOME/"qt_temp/qt-everywhere-opensource-src-5.6.0/qtbase/src/plugins/platforms/cocoa/qnsview.h"
#cp qt_changed_files/qnsview.mm $HOME/"qt_temp/qt-everywhere-opensource-src-5.6.0/qtbase/src/plugins/platforms/cocoa/qnsview.mm"
#cp qt_changed_files/qmake.conf $HOME/"qt_temp/qt-everywhere-opensource-src-5.6.0/qtbase/mkspecs/macx-clang/qmake.conf"

pushd $HOME/"qt_temp/qt-everywhere-src-5.12.7"

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
