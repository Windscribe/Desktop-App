#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

CUR_PATH=$(PWD)

#(time in seconds, default 600 = 10 minutes)
APPLE_NOTARIZE_TIMEOUT=600
APP_BUNDLE="com.windscribe.installer.macos"
APPLE_ID_EMAIL="yegor@windscribe.com"
APPLE_ID_PASSWORD="omum-ikfp-msed-negh"

echo "Compressing kext"
pushd "$CUR_PATH/binaries/installer"
/usr/bin/ditto -c -k --keepParent "WindscribeInstaller.app" "WindscribeInstaller.zip"

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

    sleep 5
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
