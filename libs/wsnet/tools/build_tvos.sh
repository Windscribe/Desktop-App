#!/usr/bin/env bash

export VCPKG_ROOT="/Users/aaa/vcpkg"
export PATH=/opt/homebrew/bin:$PATH

ARCHITECTURES="TVOS"

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
NumberOfCores=$(sysctl -n hw.ncpu)

if [ ! -d "$VCPKG_ROOT" ]; then
  echo "$VCPKG_ROOT does not exist."
  exit 1
fi

#Apply vcpkg patches for tvOS
cwd=$(pwd)
cd "$VCPKG_ROOT"
git apply "$SCRIPT_DIR/../cmake/tvos_vcpkg.patch"
cd "$cwd"

rm -r temp
rm -r bin/ios

for arch in $ARCHITECTURES; do
  rm -r ../generated
  mkdir -p "temp/build/$arch"

  cmake -B "temp/build/$arch" -S .. -G Xcode -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$SCRIPT_DIR/../cmake/ios.toolchain.cmake -DVCPKG_TARGET_TRIPLET=arm64-tvos -DPLATFORM=$arch -DDEPLOYMENT_TARGET=12 -DSCAPIX_BRIDGE=objc -DCMAKE_BUILD_TYPE=Release
  cmake --build "temp/build/$arch" -j $NumberOfCores --config Release

  cmake --install "temp/build/$arch" --prefix "bin/ios/$arch"
done

rm -r temp
