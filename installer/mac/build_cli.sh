#!/bin/bash

CUR_PATH=$(PWD)
QMAKE_PATH="$HOME/Qt/bin/qmake"
MACDEPLOY_PATH="$HOME/Qt/bin/macdeployqt"
CLI_PROJECT_PATH="${PWD}/../../gui/cli/cli.pro"

rm -rf "$CUR_PATH/binaries/cli"
mkdir -p "$CUR_PATH/binaries/cli"
pushd "$CUR_PATH/binaries/cli"

$QMAKE_PATH $CLI_PROJECT_PATH CONFIG+=release
make -j8
retn_code=$?
if [ "$retn_code" -ne "0" ]; then
	echo "CLI project build failed"
	popd
	rm -r -f "$CUR_PATH/binaries/cli"
   	exit 1;
fi

rm *.* Makefile .qmake.stash

popd

echo "windscribe-cli successfully built"

exit 0