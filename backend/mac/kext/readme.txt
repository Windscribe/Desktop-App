Windscribe kext sources.

- For build run build_kext.sh with root. The kext binary is placed in the subfolder Binary.
  - Make sure you replace $SIGNING_IDENTITY in this script with your Apple code siging certificate name
    (e.g. Developer ID Application: Windscribe Limited (ABCDEFGHIJ)) before running it.

- For Apple notarization run notarize_kext.sh script and wait for result (this may take up to 5 minutes).
