#!/usr/bin/env bash

export JAVA_HOME="/Applications/Android Studio.app/Contents/jbr/Contents/Home"
if [ ! -d "$ANDROID_NDK_HOME" ]; then
  export ANDROID_NDK_HOME="/Users/aaa/Library/Android/sdk/ndk/25.2.9519653"
fi

if [ ! -d "$VCPKG_ROOT" ]; then
  export VCPKG_ROOT="/Users/aaa/vcpkg"
fi

export PATH=/opt/homebrew/bin:$PATH

ARCHITECTURES="arm64-v8a"

if [ ! -d "$JAVA_HOME" ]; then
  echo "$JAVA_HOME does not exist."
  exit 1
fi

if [ ! -d "$ANDROID_NDK_HOME" ]; then
  echo "$ANDROID_NDK_HOME does not exist."
  exit 1
fi

if [ ! -d "$VCPKG_ROOT" ]; then
  echo "$VCPKG_ROOT does not exist."
  exit 1
fi

NumberOfCores=$(sysctl -n hw.ncpu)

rm -r temp
rm wsnet.aar

for arch in $ARCHITECTURES; do
  rm -r ../generated

  mkdir -p "temp/build/$arch"

  cmake -B "temp/build/$arch" -S .. -DANDROID_ABI=$arch -DANDROID_PLATFORM=android-21 -DSCAPIX_BRIDGE=java -DVCPKG_TARGET_ANDROID=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build "temp/build/$arch" -j $NumberOfCores

  cmake --install "temp/build/$arch" --prefix "temp/jni/$arch"
done

cp -r ../generated/bridge/java/com temp
find temp/com -name "*.java" -print | xargs "$JAVA_HOME/bin/javac" -d temp/build_java
"$JAVA_HOME/bin/jar" cfvM temp/classes.jar -C temp/build_java .

rm -r temp/build
rm -r temp/build_java
rm -r temp/com

cp AndroidManifest.xml temp/AndroidManifest.xml

(cd temp && zip -r ../wsnet.aar .)
rm -r temp


