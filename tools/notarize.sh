#!/bin/bash

ABSOLUTE_PATH_TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ABSOLUTE_PATH_INSTALLER_BINS="$ABSOLUTE_PATH_TOOLS/../temp/installer/InstallerFiles/installer"
NOTARIZE_YML=$ABSOLUTE_PATH_TOOLS/notarize.yml

#(time in seconds, default 600 = 10 minutes)
APPLE_NOTARIZE_TIMEOUT=600

# move to dir
pushd "$ABSOLUTE_PATH_INSTALLER_BINS"
if [ $? -ne 0 ]; then
    echo "Installer binary folder does not exist"
    exit 1
fi

# check that installer exists
if [ -d "WindscribeInstaller.app" ]; then
    echo "WindscribeInstaller.app exists"
else
    echo "WindscribeInstaller.app does not exist"
    exit 1
fi

# get yml variables
APP_BUNDLE="com.windscribe.installer.macos"
line=($(cat $NOTARIZE_YML | grep "apple-id-email:"))
APPLE_ID_EMAIL=${line[1]}
line=($(cat $NOTARIZE_YML | grep "apple-id-password:"))
APPLE_ID_PASSWORD=${line[1]}

echo "Compressing installer"
ditto -c -k --keepParent "WindscribeInstaller.app" "WindscribeInstaller.zip"

echo "Sending to Apple for notarization"

# Outputs are written to stderr
# Upload the tool for notarization
# (Tee through /dev/stderr so the output is logged in case we exit here due to set -e)
notarizeOutput=$( (xcrun notarytool submit "WindscribeInstaller.zip" --wait --apple-id "$APPLE_ID_EMAIL" --team-id "$1" --password "$APPLE_ID_PASSWORD") 2>&1 | tee /dev/stderr)

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
stapleOutput=$( (xcrun stapler staple "WindscribeInstaller.app") 2>&1)

if [[ $stapleOutput == *"action worked"* ]];
then
    echo "Staple action worked!"
else
    echo "Staple action failed"
    echo "$stapleOutput"
    popd
    exit 3
fi

rm "WindscribeInstaller.zip"

popd
