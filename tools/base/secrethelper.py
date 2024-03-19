#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# purpose: secret helper for build_all

import utils as utl
import pathhelper


def cleanup_secrets():
    utl.RemoveDirectory(pathhelper.mac_provision_profile_folder_name_absolute())
