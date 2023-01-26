#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# purpose: secret helper for build_all

import os

from . import utils as utl
from . import messages as msg
from . import process as proc
from . import pathhelper
from . import arghelper


def download_apps_team_file_from_1password(filename, download_filename, session_token):
    get_1p_file_cmd = ["op", "get", "document", filename, "--vault", "Apps Team", "--session", session_token]
    file_contents = proc.ExecuteAndGetOutput(get_1p_file_cmd)
    utl.CreateFileWithContents(download_filename, file_contents, True)


def login_and_download_files_from_1password(arghelper):
    # login and download file from 1password vault
    one_password_session_token = arghelper.one_password_session_token()  # could be empty string
    if not one_password_session_token:
        signing_1p_cmd = ["op", "signin", "windscribe.1password.com", arghelper.one_password_username(), "--raw"]
        one_password_session_token = proc.ExecuteAndGetOutput(signing_1p_cmd)
    msg.Print("Using Session Token: " + one_password_session_token)

    # get hardcoded secrets
    download_apps_team_file_from_1password(pathhelper.HARDCODED_SECRETS_INI,
                                           pathhelper.hardcoded_secrets_filename_absolute(),
                                           one_password_session_token)
    current_os = utl.GetCurrentOS()

    # get notarize.yml -- used by windows and mac
    if current_os != "linux":
        download_apps_team_file_from_1password(pathhelper.NOTARIZE_YML,
                                               pathhelper.notarize_yml_filename_absolute(),
                                               one_password_session_token)

    # get windows signing cert
    if current_os == "win32":
        download_apps_team_file_from_1password("Windscribe Code Sign Certificate (WINDOWS)",
                                               pathhelper.windows_signing_cert_filename(),
                                               one_password_session_token)

    # get mac provision profile
    if current_os == "macos":
        if not os.path.exists(pathhelper.mac_provision_profile_folder_name_absolute()):
            utl.CreateDirectory(pathhelper.mac_provision_profile_folder_name_absolute())
        download_apps_team_file_from_1password(pathhelper.PROVISION_PROFILE,
                                          pathhelper.mac_provision_profile_filename_absolute(),
                                          one_password_session_token)
  
    # get/extract linux keys
    if current_os == "linux":
        download_apps_team_file_from_1password("Linux Signature Private Key",
                                          pathhelper.linux_private_key_filename_absolute(),
                                          one_password_session_token)
        # extract public key from private key
        extract_pub_key_cmd = ["openssl", "rsa", "-in", pathhelper.linux_private_key_filename_absolute(), "-pubout"]
        public_key_contents = proc.ExecuteAndGetOutput(extract_pub_key_cmd)
        utl.CreateFileWithContents(pathhelper.linux_public_key_filename_absolute(), public_key_contents, True)


def cleanup_secrets():
    utl.RemoveFile(pathhelper.notarize_yml_filename_absolute())
    utl.RemoveFile(pathhelper.windows_signing_cert_filename())
    utl.RemoveDirectory(pathhelper.mac_provision_profile_folder_name_absolute())
    utl.RemoveFile(pathhelper.linux_private_key_filename_absolute())
    utl.RemoveFile(pathhelper.linux_public_key_filename_absolute())
    utl.RemoveFile(pathhelper.linux_include_key_filename_absolute())
    utl.RemoveFile(pathhelper.hardcoded_secrets_filename_absolute())
