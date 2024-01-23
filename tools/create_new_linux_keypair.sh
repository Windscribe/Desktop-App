#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose, etc: PLEASE READ:
purpose = "Creates new keypair for linux code-signing and deposites them in common/keys/linux"
remember = "REMEMBER: if you run this script for a public release remember to update the keypair in 1password."
warning = "WARNING: This script should only be run to invalidate the previous keypair on a public release. \
\nIt should not be used to populate the key directory when there are none \
\n(This should happen manually by pulling the keys from Windscribe's 1password storage).\
\n...\
\nAre you sure you want to create new keypair? (yes/no)"

import os
import sys
import base.messages as msg
import base.utils as utl
import base.process as proc
import deps.installutils as iutl

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)


def create_keys():
    linux_keys = os.path.join(ROOT_DIR, "common", "keys", "linux")
    private_key = os.path.join(linux_keys, "key.pem")
    public_key = os.path.join(linux_keys, "key.pub")

    msg.Print("Creating private key")
    iutl.RunCommand(["openssl", "genrsa", "-out", private_key, "4096"])
    msg.Print("Extracting public key")
    public_key_string = proc.ExecuteAndGetOutput(["openssl", "rsa", "-in", private_key, "-pubout"])
    utl.CreateFileWithContents(public_key, public_key_string, True)


def success_print():
    msg.Print(".\n.\n.\n")
    msg.Print("Keys created in common/keys/linux")
    msg.Print("Seriously...")
    msg.Print(remember)


if __name__ == "__main__":
    if (utl.GetCurrentOS() != "linux"):
        msg.Print("This script is not for any OS other than linux, silly!")
        sys.exit(0)

    # warn users and get confirmation
    msg.Print(purpose)
    msg.Print(remember)
    user_input = raw_input(warning)

    # user decides to not run
    if user_input != "yes":
        msg.Print("NOT creating keys! Quitting.")
        sys.exit(0)

    # proceed with key creation
    msg.Print("Creating keys")
    create_keys()
    success_print()
    sys.exit(0)
