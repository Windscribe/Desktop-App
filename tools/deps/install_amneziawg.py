#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2026, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs AmneziaWG executables.
import os
import platform
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

DEP_TITLE = "AmneziaWG"

REPO_TITLE_WIN = "amneziawg-windows"
REPO_URL_WIN = "https://github.com/amnezia-vpn/amneziawg-windows.git"
REPO_TAG_WIN = '0.1.8'

REPO_TITLE_GNU = "amneziawg-go"
REPO_URL_GNU = "https://github.com/amnezia-vpn/amneziawg-go.git"
REPO_TAG_GNU = '0.2.16'


def BuildDependencyWindows(product_name, configured_go):
    buildenv = os.environ.copy()
    outpath_amd64 = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), iutl.GetBuildLibsRoot(product_name), "wireguard"))
    outpath_arm64 = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), iutl.GetBuildLibsRoot(product_name, is_arm64=True), "wireguard"))
    if configured_go:
        dest_go = os.path.join(os.getcwd(), "conf", "path_windows.go")
        utl.CopyFile(configured_go, dest_go)
    iutl.RunCommand(["build.cmd"], env=buildenv, shell=True)
    currend_wd = os.getcwd()
    utl.CopyFile(f"{currend_wd}/x64/tunnel.dll", f"{outpath_amd64}/amneziawgtunnel.dll")
    utl.CopyFile(f"{currend_wd}/arm64/tunnel.dll", f"{outpath_arm64}/amneziawgtunnel.dll")
    msg.Print(f"{REPO_TITLE_WIN} v{REPO_TAG_WIN} installed in {outpath_amd64} and {outpath_arm64}")


def BuildDependencyMacOS(product_name):
    outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), iutl.GetBuildLibsRoot(product_name), "wireguard"))
    exe_base = "{}amneziawg".format(product_name)
    arm64_target = "{}/{}-arm64".format(outpath, exe_base)
    amd64_target = "{}/{}-amd64".format(outpath, exe_base)
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64", "GOOS": "darwin"})
    iutl.RunCommand(["make"], env=buildenv)
    utl.CopyFile("{}/amneziawg-go".format(os.getcwd()), arm64_target)
    utl.RemoveFile("{}/amneziawg-go".format(os.getcwd()))
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "amd64", "GOOS": "darwin"})
    iutl.RunCommand(["make"], env=buildenv)
    utl.CopyFile("{}/amneziawg-go".format(os.getcwd()), amd64_target)
    iutl.RunCommand([
        "lipo",
        "-create",
        amd64_target,
        arm64_target,
        "-output",
        "{}/{}".format(outpath, exe_base)])
    utl.RemoveFile(arm64_target)
    utl.RemoveFile(amd64_target)


def BuildDependencyLinux(product_name):
    outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), iutl.GetBuildLibsRoot(product_name), "wireguard"))
    exe_name = "{}amneziawg".format(product_name)
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64" if platform.machine() in ("arm64", "aarch64") else "amd64", "GOOS": "linux"})
    iutl.RunCommand(["make"], env=buildenv)
    utl.CopyFile("{}/amneziawg-go".format(os.getcwd()), "{}/{}".format(outpath, exe_name))
    iutl.RunCommand(["strip", "--strip-all", "{}/{}".format(outpath, exe_name)], env=buildenv)


def InstallDependency():
    product_name = iutl.ParseProductArg()
    isWindowsBuild = current_os == "win32"
    dep_name = DEP_TITLE.lower()
    temp_dir = iutl.PrepareTempDirectory(dep_name)
    configured_go = None
    with utl.PushDir(temp_dir):
        iutl.RunCommand(["git", "clone", (REPO_URL_WIN if isWindowsBuild else REPO_URL_GNU), "."])
        iutl.RunCommand(["git", "checkout", "tags/v{}".format(REPO_TAG_WIN if isWindowsBuild else REPO_TAG_GNU)])
        if isWindowsBuild:
            template_in = os.path.join(TOOLS_DIR, "deps", "custom_amneziawg-windows", "conf", "path_windows.go.in")
            configured_go = os.path.join(os.path.dirname(temp_dir), "path_windows.go")
            iutl.ConfigureDependencyTemplate(product_name, template_in, configured_go)
            iutl.CopyCustomFiles(REPO_TITLE_WIN, temp_dir)
            BuildDependencyWindows(product_name, configured_go)
        elif current_os == "macos":
            BuildDependencyMacOS(product_name)
        else:
            BuildDependencyLinux(product_name)
    msg.Print("Cleaning temporary directory...")
    utl.RemoveDirectory(temp_dir)


if __name__ == "__main__":
    start_time = time.time()
    current_os = utl.GetCurrentOS()
    try:
        msg.Print("Installing {}...".format(DEP_TITLE))
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
