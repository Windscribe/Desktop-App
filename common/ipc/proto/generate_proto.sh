#!/bin/bash

CUR_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "$CUR_PATH"
if [ -d "$CUR_PATH/../generated_proto" ]; then rm -rf "$CUR_PATH/../generated_proto"; fi
mkdir -p ../generated_proto

protodir=$1
[ -z "$protodir" ] && protodir=$CUR_PATH/../../../build-libs/protobuf/bin

$protodir/protoc -I=$CUR_PATH --cpp_out=$CUR_PATH/../generated_proto $CUR_PATH/*.proto
