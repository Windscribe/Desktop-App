#!/bin/bash

CUR_PATH=$(PWD)
LAUNCHER_PROJECT_PATH="${PWD}/../../gui/launcher/mac"

rm -rf "$CUR_PATH/binaries/launcher"
pushd $LAUNCHER_PROJECT_PATH

xcodebuild -scheme WindscribeLauncher -configuration Release clean build || exit 1

OUT_DIR=$(xcodebuild -project windscribelauncher.xcodeproj -showBuildSettings | grep -m 1 "BUILT_PRODUCTS_DIR" | grep -oEi "\/.*")
mkdir -p "$CUR_PATH/binaries/launcher"
cp -a "${OUT_DIR}/WindscribeLauncher.app" "$CUR_PATH/binaries/launcher/WindscribeLauncher.app"

popd

echo "WindscribeLauncher.app successfully created"
exit 0