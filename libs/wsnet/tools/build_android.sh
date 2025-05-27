#!/usr/bin/env bash

set -e  # Exit on any error
set -o pipefail

ARCHITECTURES=("arm64-v8a" "armeabi-v7a" "x86" "x86_64")

# Use nproc (Linux) instead of sysctl (macOS)
NumberOfCores=$(nproc || echo 4)

# Clean up safely
[ -d temp ] && rm -rf temp
[ -f wsnet.aar ] && rm -f wsnet.aar

for arch in "${ARCHITECTURES[@]}"; do
  [ -d ../generated ] && rm -rf ../generated
  mkdir -p "temp/build/$arch"
  cmake -B "temp/build/$arch" -S .. \
    -DANDROID_ABI=$arch \
    -DANDROID_PLATFORM=android-21 \
    -DSCAPIX_BRIDGE=java \
    -DVCPKG_TARGET_ANDROID=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -G Ninja

  cmake --build "temp/build/$arch" -j "$NumberOfCores"
  cmake --install "temp/build/$arch" --prefix "temp/jni/$arch"
done

# Compile Java bridge
cp -r ../generated/bridge/java/com temp || { echo "Java bridge sources missing"; exit 1; }

mkdir -p temp/build_java
find temp/com -name "*.java" -print | xargs "$JAVA_HOME/bin/javac" -d temp/build_java

"$JAVA_HOME/bin/jar" cfvM temp/classes.jar -C temp/build_java .
rm -r temp/build
rm -r temp/build_java
rm -r temp/com
# Bundle into AAR
cp AndroidManifest.xml temp/AndroidManifest.xml
(cd temp && zip -r ../wsnet.aar .)
rm -rf temp