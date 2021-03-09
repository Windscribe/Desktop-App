#!/bin/bash

CUR_PATH=$(PWD)
QMAKE_PATH="$HOME/Qt/bin/qmake"
MACDEPLOY_PATH="$HOME/Qt/bin/macdeployqt"
ENGINE_PROJECT_PATH="$CUR_PATH/../../backend/engine/engine.pro"

rm -rf "$CUR_PATH/binaries/engine"
mkdir -p "$CUR_PATH/binaries/engine"
pushd "$CUR_PATH/binaries/engine"
$QMAKE_PATH $ENGINE_PROJECT_PATH CONFIG+=release
make -j8
retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "Engine project build failed"
	popd
	rm -r -f "$CUR_PATH/binaries/engine"
   	exit 1;
fi

rm * .*

$MACDEPLOY_PATH WindscribeEngine.app -no-plugins

# copy openssl libs to WindscribeEngine.app/Contents/Helpers
cp $HOME/LibsWindscribe/openssl/lib/libcrypto.1.1.dylib WindscribeEngine.app/Contents/Helpers/libcrypto.1.1.dylib
cp $HOME/LibsWindscribe/openssl/lib/libssl.1.1.dylib WindscribeEngine.app/Contents/Helpers/libssl.1.1.dylib

# update linking paths (otherwise it won't work on another Mac)
pushd WindscribeEngine.app/Contents/Helpers
install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libssl.1.1.dylib @executable_path/libssl.1.1.dylib windscribeopenvpn_2_5_0
install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib windscribeopenvpn_2_5_0

install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libssl.1.1.dylib @executable_path/libssl.1.1.dylib windscribestunnel
install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib windscribestunnel

install_name_tool -change $HOME/LibsWindscribe/openssl/lib/libcrypto.1.1.dylib @executable_path/libcrypto.1.1.dylib libssl.1.1.dylib

popd
popd

python update_plist_version.py "$CUR_PATH/binaries/engine/WindscribeEngine.app/Contents/info.plist" "$CUR_PATH/../../common/version/windscribe_version.h"

codesign --deep "$CUR_PATH/binaries/engine/WindscribeEngine.app"  -s "Developer ID Application: Windscribe Limited (GYZJYS7XUG)" --options runtime --timestamp
retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "Engine project signing failed"
	exit 1;
fi

codesign --entitlements "$CUR_PATH/../../backend/engine/mac/windscribe_engine.entitlements" -f -s "Developer ID Application: Windscribe Limited (GYZJYS7XUG)" --options runtime --timestamp "$CUR_PATH/binaries/engine/WindscribeEngine.app/Contents/MacOS/WindscribeEngine"
retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "Engine project signing failed"
	exit 1;
fi
exit 0
