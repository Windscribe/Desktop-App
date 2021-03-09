#!/bin/bash

CUR_PATH=$(PWD)

verStr=$(python update_plist_version.py "$CUR_PATH/../../common/version/windscribe_version.h")

#update protobuf
pushd $CUR_PATH/../../common/ipc/proto
./generate_proto.sh
popd

./build_helper.sh $1
./build_engine.sh $1
./build_launcher.sh $1
./build_cli.sh $1
./build_gui.sh $1
./build_installer.sh $1

pushd "$CUR_PATH/binaries/installer"
dropdmg --config-name="Windscribe2" WindscribeInstaller.app
popd

mv "$CUR_PATH/binaries/installer/WindscribeInstaller.dmg" Windscribe_$verStr.dmg