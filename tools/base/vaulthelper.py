#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: help with working with vault

import json
import sys
from . import utils as utl


class VaultParser:
    def __init__(self):
        pass

    # used to get raw bytes from the json string
    @staticmethod
    def parse_vault_json_raw(first, second):
        json_entry = json.dumps(json.load(sys.stdin)[first][second])
        json_entry = json_entry.strip("\"")  # remove quotes around the raw text
        return json_entry

    # used for unpacking text-bytes from json
    @staticmethod
    def parse_vault_json(first, second):
        json_entry = VaultParser.parse_vault_json_raw(first, second)

        # on windows we can replace the actual characters "\" and "n" with newline
        # macos and linux use echo -e to interpret the "\n" as actual newlines
        if utl.GetCurrentOS() == utl.CURRENT_OS_WIN:
            json_split = json_entry.split("\\n")
            json_entry = "\n".join(json_split)

        # Replace escaped double-quotes with just double quotes
        return json_entry.replace("\\\"", "\"")

    @staticmethod
    def client_token():
        sys.stdout.write(VaultParser.parse_vault_json('auth', 'client_token'))

    @staticmethod
    def notarize_yml():
        notarize_contents = VaultParser.parse_vault_json('data', 'notarize.yml')
        sys.stdout.write(notarize_contents) 

    @staticmethod
    def hardcoded_secrets():
        hardcoded_contents = VaultParser.parse_vault_json('data', 'hardcodedsecrets.ini')
        sys.stdout.write(hardcoded_contents) 

    @staticmethod
    def windows_cert():
        base64_encoded_cert = VaultParser.parse_vault_json('data', 'CODE_SIGN_CERT_WIN')
        sys.stdout.write(base64_encoded_cert)

    @staticmethod
    def linux_priv_key():
        sys.stdout.write(VaultParser.parse_vault_json('data', 'CODE_SIGN_PRIV_KEY_LINUX'))

    @staticmethod
    def mac_provision_profile():
        sys.stdout.write(VaultParser.parse_vault_json_raw('data', 'CODE_SIGN_PROVISION_PROFILE_MAC'))


if __name__ == "__main__":
    if "--get-client-token" in sys.argv:
        VaultParser.client_token()
    elif "--get-hardcoded-secrets" in sys.argv:
        VaultParser.hardcoded_secrets()
    elif "--get-windows-cert" in sys.argv:
        VaultParser.windows_cert()
    elif "--get-linux-priv-key" in sys.argv:
        VaultParser.linux_priv_key()
    elif "--get-mac-provision-profile" in sys.argv:
        VaultParser.mac_provision_profile()
    elif "--get-notarize-yml" in sys.argv:
        VaultParser.notarize_yml()
    else:
        print("Error, pass argument that specifies what to parse for")
        sys.exit(1)
