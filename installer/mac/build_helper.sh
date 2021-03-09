#!/bin/bash

CUR_PATH=$(PWD)
HELPER_PROJECT_PATH="${PWD}/../../backend/mac/helper"

COMPILER_DEFINES=
COMPILER_PARMS=
if [ "$1" == "private" ]; then
  COMPILER_DEFINES="-DDISABLE_HELPER_SECURITY_CHECK=1"
fi
if [ -n "$COMPILER_DEFINES" ]; then
  COMPILER_PARMS="OTHER_CFLAGS=\"$COMPILER_DEFINES\""
fi

rm -rf "$CUR_PATH/binaries/helper"
pushd $HELPER_PROJECT_PATH

xcodebuild -scheme com.windscribe.helper.macos "OTHER_CODE_SIGN_FLAGS=--timestamp" "CODE_SIGN_INJECT_BASE_ENTITLEMENTS=NO" -configuration Release $COMPILER_PARMS clean build || exit 1

OUT_DIR=$(xcodebuild -project helper.xcodeproj -showBuildSettings | grep -m 1 "BUILT_PRODUCTS_DIR" | grep -oEi "\/.*")
mkdir -p "$CUR_PATH/binaries/helper"
cp -a "${OUT_DIR}/com.windscribe.helper.macos" "$CUR_PATH/binaries/helper/com.windscribe.helper.macos"

popd
echo "Helper successfully created"
exit 0