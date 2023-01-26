#! /usr/bin/env bash

set -x

rm="git rm --ignore-unmatch"

find . -not -path "./.git/*" '(' -name '*.exe' -or -name '*.dll' ')' | xargs $rm
grep -rIL . | grep -v "^\.git/" | grep -v -E '.(png|jpg|tiff|ico|icns|ttf)$' | xargs $rm
$rm tools/bin/get-pip.py
