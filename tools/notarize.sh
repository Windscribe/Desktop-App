#!/bin/bash

ABSOLUTE_PATH_TOOLS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ABSOLUTE_PATH_INSTALLER_BINS="$ABSOLUTE_PATH_TOOLS/../installer/mac/binaries/installer"
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
notarizeOutput=$( (xcrun altool --notarize-app -t osx -f "WindscribeInstaller.zip" --primary-bundle-id="$APP_BUNDLE" -u "$APPLE_ID_EMAIL" -p "$APPLE_ID_PASSWORD") 2>&1 | tee /dev/stderr)

if [[ $notarizeOutput == *"No errors uploading"* ]];
then
    REQUEST_ID=$(echo "$notarizeOutput"| grep RequestUUID |awk '{print $NF}')
    echo "Request ID is: $REQUEST_ID"
else
    echo "Notarization upload failed."
    echo "$notarizeOutput"
    popd
    exit 3
fi

# Wait 20 seconds otherwise we may get the error "Could not find the RequestUUID"
echo "Waiting 20 seconds"
sleep 20

# Wait up to 10 minutes
deadline=$(($(date "+%s") + ${APPLE_NOTARIZE_TIMEOUT}))

while :
do
    notarizationStatus=$( (xcrun altool --notarization-info "$REQUEST_ID" -u "$APPLE_ID_EMAIL" -p "$APPLE_ID_PASSWORD") 2>&1 | tee /dev/stderr)

    if [[ $notarizationStatus == *"in progress"* ]];
    then
        now="$(date "+%s")"
        if [[ "$now" -ge "$deadline" ]];
        then
            echo "Timed out waiting for approval"
            echo "Last status:"
            echo "$notarizationStatus"
            popd
            exit 3
        fi
        # In progress
        echo "Waiting for notarization"
    elif [[ $notarizationStatus == *"Package Approved"* ]];
    then
        echo "Package has been approved"
        break;
    elif [[ $notarizationStatus == *"invalid"* ]];
    then
        echo "Notarization invalid"
        echo "$notarizationStatus"
        popd
        exit 3
    else
        echo "Unknown notarization status"
        echo "$notarizationStatus"
    fi

    sleep 15
done

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
