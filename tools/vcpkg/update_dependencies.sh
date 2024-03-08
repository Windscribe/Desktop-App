#!/bin/bash

#This script installs or updates all dependencies locally

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
"${VCPKG_ROOT}/vcpkg" install --x-install-root=${VCPKG_ROOT}/installed --x-manifest-root=${SCRIPT_DIR}