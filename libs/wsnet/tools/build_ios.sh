#!/usr/bin/env bash
VERSION="1.0.0"
export PATH=/opt/homebrew/bin:$PATH

ARCHITECTURES=("SIMULATORARM64" "OS64" "TVOS" "SIMULATORARM64_TVOS")
TRIPLETS=("arm64-ios-simulator" "arm64-ios" "arm64-tvos" "arm64-tvos-simulator")

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
NumberOfCores=$(sysctl -n hw.ncpu)

updateInfoPlist() {
  plistFile=$1
  /usr/libexec/PlistBuddy -c "add :CFBundleShortVersionString string $VERSION" "$plistFile"
  /usr/libexec/PlistBuddy -c "add :CFBundleVersion string $VERSION" "$plistFile"
}

# Clean up previous builds
rm -rf temp
rm -rf bin/ios

# Build for each architecture
# shellcheck disable=SC2068
for i in ${!ARCHITECTURES[@]}; do
  arch=${ARCHITECTURES[$i]}
  triplet=${TRIPLETS[$i]}
  deploymentTarget=12
  if [ "$arch" == "TVOS" ] || [ "$arch" == "SIMULATORARM64_TVOS" ]; then
      deploymentTarget=17
  fi

  rm -rf "../generated"
  mkdir -p "temp/build/$arch"

  cmake -B "temp/build/$arch" -S .. \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$SCRIPT_DIR/../cmake/ios.toolchain.cmake \
    -DVCPKG_TARGET_TRIPLET=$triplet \
    -DPLATFORM=$arch \
    -DDEPLOYMENT_TARGET=$deploymentTarget \
    -DSCAPIX_BRIDGE=objc \
    -DCMAKE_BUILD_TYPE=Release
  cmake --build "temp/build/$arch" -j $NumberOfCores --config Release
  cmake --install "temp/build/$arch" --prefix "bin/ios/$arch"
   # Add missing keys to Info.plist.
     updateInfoPlist "bin/ios/$arch/wsnet.framework/Info.plist"
done

# Build combined framework
xcodebuild -create-xcframework \
   -framework ./bin/ios/OS64/wsnet.framework \
   -framework ./bin/ios/SIMULATORARM64/wsnet.framework \
   -framework ./bin/ios/TVOS/wsnet.framework \
   -framework ./bin/ios/SIMULATORARM64_TVOS/wsnet.framework \
   -output ./build/WSNet.xcframework

rm -rf ./bin
rm -rf temp