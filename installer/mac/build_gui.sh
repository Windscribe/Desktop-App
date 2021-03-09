#!/bin/bash

CUR_PATH=$(PWD)
QMAKE_PATH="$HOME/Qt/bin/qmake"
MACDEPLOY_PATH="$HOME/Qt/bin/macdeployqt"
GUI_PROJECT_PATH="${PWD}/../../gui/gui/gui.pro"

rm -rf "$CUR_PATH/binaries/gui"
mkdir -p "$CUR_PATH/binaries/gui"
pushd "$CUR_PATH/binaries/gui"

$QMAKE_PATH $GUI_PROJECT_PATH CONFIG+=release
make -j8
retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "GUI project build failed"
	popd
	rm -r -f "$CUR_PATH/binaries/gui"
   	exit 1;
fi

rm * .*

$MACDEPLOY_PATH Windscribe.app
#remove unused Qt frameworks
rm -rf Windscribe.app/Contents/Frameworks/QtQml.framework
rm -rf Windscribe.app/Contents/Frameworks/QtQuick.framework

# point windscribe-cli toward protobuf lib
pushd Windscribe.app/Contents/MacOS
install_name_tool -change $HOME/LibsWindscribe/protobuf/lib/libprotobuf.19.dylib @executable_path/../Frameworks/libprotobuf.19.dylib windscribe-cli
popd

pushd Windscribe.app/Contents/Library
install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libssl.1.1.dylib @executable_path/../Frameworks/libssl.1.1.dylib wsappcontrol
install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libcrypto.1.1.dylib @executable_path/../Frameworks/libcrypto.1.1.dylib wsappcontrol
popd

popd

python update_plist_version.py "$CUR_PATH/binaries/gui/Windscribe.app/Contents/Info.plist" "$CUR_PATH/../../common/version/windscribe_version.h"

codesign --deep "$CUR_PATH/binaries/gui/Windscribe.app"  -s "Developer ID Application: Windscribe Limited (GYZJYS7XUG)" --options runtime --timestamp
retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "Gui project signing failed"
	exit 1;
fi

exit 0