#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
import os

# Hierarchy tree (linux):
# client-desktop                      (ROOT_DIR)
#     build-exe                       (artifact_dir)
#     common                          (COMMON_DIR)
#     installer
#         linux                       (LINUX_INSTALLER_ROOT)
#             debian_package          (src_package_path)
#     temp
#         installer
#             InstallerFiles          (BUILD_INSTALLER_FILES)
#                 signatures
#             windscribe_<version>_amd64  (dest_package_path)


# directory structure paths
BASE_DIR = os.path.dirname(os.path.abspath(__file__))  # tools/base
TOOLS_DIR = os.path.dirname(BASE_DIR)
ROOT_DIR = os.path.dirname(TOOLS_DIR)
COMMON_DIR = os.path.join(ROOT_DIR, "client/common")
TEMP_DIR = os.path.join(ROOT_DIR, "temp")
TEMP_INSTALLER_DIR = os.path.join(TEMP_DIR, "installer")

# some filenames
NOTARIZE_YML = "notarize.yml"
NOTARIZE_SCRIPT = "notarize.sh"
PROVISION_PROFILE = "embedded.provisionprofile"


def windows_signing_cert_filename():
    return os.path.join(ROOT_DIR, "installer", "windows", "signing", "code_signing.pfx")


def notarize_yml_filename_absolute():
    return os.path.join(TOOLS_DIR, NOTARIZE_YML)


def notarize_script_filename_absolute():
    return os.path.join(TOOLS_DIR, NOTARIZE_SCRIPT)


def mac_provision_profile_folder_name_absolute():
    return os.path.join(ROOT_DIR, "backend", "mac", "provisioning_profile")


def mac_provision_profile_filename_absolute():
    return os.path.join(mac_provision_profile_folder_name_absolute(), PROVISION_PROFILE)


def linux_key_directory():
    return os.path.join(COMMON_DIR, "keys", "linux")


def linux_private_key_filename_absolute():
    return os.path.join(linux_key_directory(), "key.pem")


def linux_public_key_filename_absolute():
    return os.path.join(linux_key_directory(), "key.pub")


def linux_include_key_filename_absolute():
    return os.path.join(linux_key_directory(), "key_pub.txt")
