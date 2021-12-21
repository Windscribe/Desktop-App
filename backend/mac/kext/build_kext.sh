#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi


CUR_PATH=$(PWD)
KEXT_PROJECT_PATH="${PWD}"

rm -rf "$CUR_PATH/Binary"
pushd $KEXT_PROJECT_PATH

xcodebuild -scheme WindscribeKext -configuration Release clean build || exit 1

OUT_DIR=$(xcodebuild -project windscribe_kext.xcodeproj -showBuildSettings | grep -m 1 "BUILT_PRODUCTS_DIR" | grep -oEi "\/.*")
mkdir -p "$CUR_PATH/Binary"
cp -a "${OUT_DIR}/WindscribeKext.kext" "$CUR_PATH/Binary/WindscribeKext.kext"


codesign --force --deep "$CUR_PATH/Binary/WindscribeKext.kext" -s "$SIGNING_IDENTITY" --options runtime --timestamp

popd
echo "Kext build succeeded"

