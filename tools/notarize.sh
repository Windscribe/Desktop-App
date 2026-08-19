#!/bin/bash

# Usage: ./notarize.sh [--dmg] <TEAM_ID> <ROOT_DIR> <BASE_NAME>
# Default:  notarizes <BASE_NAME>.app (ditto-zipped) and staples the .app.
# With --dmg: notarizes and staples <BASE_NAME>.dmg directly (a disk image is submitted as-is).

NOTARIZE_YML=$ABSOLUTE_PATH_TOOLS/notarize.yml

#(time in seconds, default 600 = 10 minutes)
APPLE_NOTARIZE_TIMEOUT=600

MODE="app"
if [ "$1" == "--dmg" ]; then
    MODE="dmg"
    shift
fi

# move to dir
pushd "$2"
if [ $? -ne 0 ]; then
    echo "Installer binary folder does not exist"
    exit 1
fi

# Determine what to submit and what to staple
if [ "$MODE" == "dmg" ]; then
    if [ -f "$3.dmg" ]; then
        echo "$3.dmg exists"
    else
        echo "$3.dmg does not exist"
        popd
        exit 1
    fi
    submitTarget="$3.dmg"
    stapleTarget="$3.dmg"
else
    if [ -d "$3.app" ]; then
        echo "$3.app exists"
    else
        echo "$3.app does not exist"
        popd
        exit 1
    fi
    echo "Compressing installer"
    ditto -c -k --keepParent "$3.app" "$3.zip"
    submitTarget="$3.zip"
    stapleTarget="$3.app"
fi

echo "Sending to Apple for notarization"

# Outputs are written to stderr
# Upload the tool for notarization
# (Tee through /dev/stderr so the output is logged in case we exit here due to set -e)
notarizeOutput=$( (xcrun notarytool submit "$submitTarget" --wait --apple-id "$APPLE_ID_EMAIL" --team-id "$1" --password "$APPLE_ID_PASSWORD") 2>&1 | tee /dev/stderr)

if [[ $notarizeOutput == *"Processing complete"* ]];
then
    echo "Notarization request processed."
else
    echo "Notarization upload failed."
    echo "$notarizeOutput"
    popd
    exit 3
fi

# Assuming that the package has been approved by this point
stapleOutput=$( (xcrun stapler staple "$stapleTarget") 2>&1)

if [[ $stapleOutput == *"action worked"* ]];
then
    echo "Staple action worked!"
else
    echo "Staple action failed"
    echo "$stapleOutput"
    popd
    exit 3
fi

# Clean up the intermediate zip only in app mode
if [ "$MODE" != "dmg" ]; then
    rm "$3.zip"
fi

popd
