#!/bin/bash
echo "Installing Google Test..."

VERSION_GTEST="1.10.0"

rm -rf $HOME/"gtest_temp"
rm -rf $HOME/"LibsWindsccribe/gtest"

mkdir $HOME/gtest_temp
curl https://github.com/google/googletest/archive/refs/tags/release-$VERSION_GTEST.tar.gz -o $HOME/"gtest_temp/gtest.tar.gz" -k -L
tar -zxvf $HOME/"gtest_temp/gtest.tar.gz" -C $HOME/"gtest_temp"

pushd $HOME/"gtest_temp/googletest-release-"$VERSION_GTEST

mkdir build
cd build
/Applications/CMake.app/Contents/bin/cmake .. -Dgtest_force_shared_crt=ON -DCMAKE_INSTALL_PREFIX=$HOME/LibsWindscribe/gtest
make
make install

# Handle additional parameters: -zip "PATH"
if [ "$#" -eq 2 ] && [ "$1" == "-zip" ]; then
	echo "Making zip into $2"
	7z a "$2"/gtest.zip "$HOME/LibsWindscribe/gtest"
fi