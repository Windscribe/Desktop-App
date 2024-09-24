#!/usr/bin/env bash

ARCHITECTURES=("arm64-v8a" "armeabi-v7a" "x86" "x86_64")

NumberOfCores=$(sysctl -n hw.ncpu)

rm -r temp
rm wsnet.aar

for arch in "${ARCHITECTURES[@]}"; do
  rm -r ../generated

  mkdir -p "temp/build/$arch"

  cmake -B "temp/build/$arch" -S .. -DANDROID_ABI=$arch -DANDROID_PLATFORM=android-21 -DSCAPIX_BRIDGE=java -DVCPKG_TARGET_ANDROID=ON -DCMAKE_BUILD_TYPE=Release 1> /dev/null
  cmake --build "temp/build/$arch" -j $NumberOfCores 1> /dev/null

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
