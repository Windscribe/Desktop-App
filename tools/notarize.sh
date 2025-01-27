#!/bin/bash

# Usage: ./notarize.sh <TEAM_ID> <ROOT_DIR> <APP_BASE_NAME>

NOTARIZE_YML=$ABSOLUTE_PATH_TOOLS/notarize.yml

#(time in seconds, default 600 = 10 minutes)
APPLE_NOTARIZE_TIMEOUT=600

# move to dir
pushd "$2"
if [ $? -ne 0 ]; then
    echo "Installer binary folder does not exist"
    exit 1
fi

# check that installer exists
if [ -d "$3.app" ]; then
    echo "$3.app exists"
else
    echo "$3.app does not exist"
    exit 1
fi

# get yml variables
echo "Compressing installer"
ditto -c -k --keepParent "$3.app" "$3.zip"

echo "Sending to Apple for notarization"

# Outputs are written to stderr
# Upload the tool for notarization
# (Tee through /dev/stderr so the output is logged in case we exit here due to set -e)
notarizeOutput=$( (xcrun notarytool submit "$3.zip" --wait --apple-id "$APPLE_ID_EMAIL" --team-id "$1" --password "$APPLE_ID_PASSWORD") 2>&1 | tee /dev/stderr)

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
stapleOutput=$( (xcrun stapler staple "$3.app") 2>&1)

if [[ $stapleOutput == *"action worked"* ]];
then
    echo "Staple action worked!"
else
    echo "Staple action failed"
    echo "$stapleOutput"
    popd
    exit 3
fi

rm "$3.zip"

popd
