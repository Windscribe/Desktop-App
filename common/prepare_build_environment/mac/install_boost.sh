#!/bin/bash

echo "Installing boost into $HOME/LibsWindscribe/boost"

rm -rf $HOME/"boost_temp"
rm -rf $HOME/"LibsWindscribe/boost"

mkdir $HOME/"boost_temp"
curl https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz -o $HOME/"boost_temp/boost_1_69_0.tar.gz" -k -L
tar -zxvf $HOME/"boost_temp/boost_1_69_0.tar.gz" -C $HOME/"boost_temp"

pushd $HOME/"boost_temp/boost_1_69_0"
sh bootstrap.sh --prefix=$HOME/"LibsWindscribe/boost" --with-toolset=clang
# only need to compile serialization, system and thread on Mac. Other libs are accessible via header-only libs.
./b2 install link=static toolset=clang cflags=-mmacosx-version-min=10.11 cxxflags=-mmacosx-version-min=10.11 mflags=-mmacosx-version-min=10.11 mmflags=-mmacosx-version-min=10.11 linkflags=-mmacosx-version-min=10.11 --with-serialization --with-system --with-thread

echo "Removing *.dylibs:"
rm -v $HOME/LibsWindscribe/boost/lib/*.dylib

popd
rm -rf $HOME/"boost_temp"

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/boost.zip "$HOME/LibsWindscribe/boost"
fi

