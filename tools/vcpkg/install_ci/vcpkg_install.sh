#!/bin/bash

# This script installs vcpkg to the home directory if that directory does not already exist.
VCPKG_PATH="$HOME/vcpkg"

if [ -z "$1" ]; then
    echo The path parameter VCPKG_PATH is not set.
    echo "Usage: vcpkg_install.sh <VCPKG_PATH> [--configure-git]"
    exit 1
fi

if [ "$2" = "--configure-git" ]; then
    echo "Configure git config for vcpkg"
    #remove previous config if found
    OUTPUT=$(git config --list | grep "insteadof=git@gitlab.int.windscribe.com" | sed "s/.insteadof=git@gitlab.int.windscribe.com.*//")
    while read line
    do
        echo "Remove previous git config for vcpkg: $line"
        git config --global --remove-section "$line"
    done < <(echo "$OUTPUT")
    git config --global url."https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.int.windscribe.com/".insteadOf "git@gitlab.int.windscribe.com:"
fi

if [ -d "$1" ]; then
    echo "vcpkg directory exists"
    #output vcpkg version
    "$1"/vcpkg version
else
    echo "vcpkg directory does not exist, install it to $1"
    mkdir -p "${1}"
    pushd "${1}" > /dev/null
    git clone https://github.com/Microsoft/vcpkg.git .
    ./bootstrap-vcpkg.sh --disableMetrics
    popd > /dev/null
fi
