#!/bin/bash

CUR_PATH=$(pwd)

rm -rf "$CUR_PATH/../generated_proto"
mkdir -p ../generated_proto

protodir=$1
[ -z "$protodir" ] && protodir=$HOME/LibsWindscribe/protobuf/bin

$protodir/protoc -I=$CUR_PATH --cpp_out=$CUR_PATH/../generated_proto $CUR_PATH/*.proto
