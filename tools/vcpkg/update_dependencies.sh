#!/bin/bash

#This script installs or updates all dependencies locally

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
# Same triplet choice as cmake/vcpkg.cmake for local builds, so the packages land where the build looks.
case "$(uname -s)-$(uname -m)" in
    Darwin-arm64)               TRIPLET=ws-arm64-osx; HOST_ARGS=() ;;
    Darwin-*)                   TRIPLET=ws-x64-osx; HOST_ARGS=() ;;
    Linux-aarch64|Linux-arm64)  TRIPLET=ws-arm64-linux; HOST_ARGS=(--host-triplet=ws-arm64-linux) ;;
    *)                          TRIPLET=ws-x64-linux; HOST_ARGS=(--host-triplet=ws-x64-linux) ;;
esac

"${VCPKG_ROOT}/vcpkg" install --x-install-root="${VCPKG_ROOT}/installed-${TRIPLET}" --x-manifest-root="${SCRIPT_DIR}" \
    --triplet="${TRIPLET}" "${HOST_ARGS[@]}" --clean-buildtrees-after-build --clean-packages-after-build