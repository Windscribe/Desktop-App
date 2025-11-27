#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs wintun DLL and header file.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "openvpn-dco.yml")

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "ovpn-dco-win"
DEP_URL = "https://github.com/OpenVPN/ovpn-dco-win/releases/download"
DEP_OS_LIST = ["win32"]


def DownloadAndCopy(dep_version_str, is_arm64):
    dep_name = DEP_TITLE.lower()
    temp_dir = iutl.PrepareTempDirectory(dep_name)
    archivename = f"{dep_name}-{dep_version_str}-"
    archivename += "arm64.zip" if is_arm64 else "amd64.zip"
    webfilename = f"{DEP_URL}/{dep_version_str}/{archivename}"
    localfilename = os.path.join(temp_dir, archivename)
    msg.HeadPrint(f"Downloading: \"{archivename}\"")
    dep_checksum_var = "CHECKSUM_" + DEP_TITLE.upper().replace("-", "_") + "_" + ("ARM64" if is_arm64 else "AMD64")
    dep_checksum_str = os.environ.get(dep_checksum_var)
    iutl.DownloadFile(webfilename, localfilename, dep_checksum_str)
    msg.HeadPrint(f"Extracting: \"{archivename}\"")
    iutl.ExtractFile(localfilename)
    dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
    dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs-arm64" if is_arm64 else "build-libs", dep_name))
    outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
    utl.CopyAllFiles(f"{temp_dir}", f"{outpath}")
    msg.Print(f"{DEP_TITLE} v{dep_version_str} installed in {outpath}")

    # Cleanup.
    msg.Print("Cleaning temporary directory...")
    utl.RemoveDirectory(temp_dir)


def InstallDependency():
    if "--arm64" in sys.argv:
        msg.Warn("--arm64 flag not required.  This script installs both amd64 and arm64 binaries on Windows.")
    msg.HeadPrint(f"Loading: \"{CONFIG_NAME}\"")
    configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
    if not configdata:
        raise iutl.InstallError("Failed to get config data.")
    iutl.SetupEnvironment(configdata)
    dep_version_var = "VERSION_" + DEP_TITLE.upper().replace("-", "_")
    dep_version_str = os.environ.get(dep_version_var, None)
    if not dep_version_str:
        raise iutl.InstallError(f"{dep_version_var} not defined.")

    DownloadAndCopy(dep_version_str, False)
    DownloadAndCopy(dep_version_str, True)


if __name__ == "__main__":
    start_time = time.time()
    current_os = utl.GetCurrentOS()
    if current_os not in DEP_OS_LIST:
        msg.Print(f"{DEP_TITLE} is not needed on {current_os}, skipping.")
        sys.exit(0)
    try:
        msg.Print(f"Installing {DEP_TITLE}...")
        InstallDependency()
        exitcode = 0
    except iutl.InstallError as e:
        msg.Error(e)
        exitcode = e.exitcode
    except IOError as e:
        msg.Error(e)
        exitcode = 1
    elapsed_time = time.time() - start_time
    if elapsed_time >= 60:
        msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time / 60, elapsed_time % 60))
    else:
        msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
    sys.exit(exitcode)
