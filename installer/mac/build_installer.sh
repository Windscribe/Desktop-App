#!/bin/bash

CUR_PATH=$(PWD)
INSTALLER_PROJECT_PATH="${PWD}/installer"

rm Windscribe.dmg
rm -rf "$CUR_PATH/binaries/installer"
mkdir -p "$CUR_PATH/binaries/installer"

# install 7z with:
# brew install p7zip

#1) make 7zip archive of Windscribe.app
rm $INSTALLER_PROJECT_PATH/installer/resources/windscribe.7z
7z a $INSTALLER_PROJECT_PATH/installer/resources/windscribe.7z $CUR_PATH/binaries/gui/Windscribe.app/  

retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "Windscribe project 7zip failed"
	popd
	rm -r -f "$CUR_PATH/binaries/installer"
   	exit 1;
fi

pushd $INSTALLER_PROJECT_PATH
xcodebuild -scheme installer "OTHER_CODE_SIGN_FLAGS=--timestamp" "CODE_SIGN_INJECT_BASE_ENTITLEMENTS=NO" -configuration Release clean build || exit 1

OUT_DIR=$(xcodebuild -project installer.xcodeproj -showBuildSettings | grep -m 1 "BUILT_PRODUCTS_DIR" | grep -oEi "\/.*")
cp -a "${OUT_DIR}/installer.app" "$CUR_PATH/binaries/installer/WindscribeInstaller.app"

popd

echo "Installer successfully created"
exit 0