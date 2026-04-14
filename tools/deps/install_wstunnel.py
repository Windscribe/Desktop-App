#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe VPN Build System
# Copyright (c) 2020-2026, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs Wstunnel executables.
import os
import platform
import sys
import time
TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "wstunnel.yml")

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "wstunnel"
DEP_URL = "https://github.com/Windscribe/wstunnel.git"
DEP_OS_LIST = ["win32", "macos", "linux"]


def BuildDependencyWindows(product_name):
    buildenv = os.environ.copy()
    outpath_amd64 = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), iutl.GetBuildLibsRoot(product_name), DEP_TITLE))
    outpath_arm64 = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), iutl.GetBuildLibsRoot(product_name, is_arm64=True), DEP_TITLE))
    iutl.RunCommand(["build.cmd"], env=buildenv, shell=True)
    exe_name = "{}wstunnel.exe".format(product_name)
    utl.CreateDirectory(outpath_amd64)
    utl.CreateDirectory(outpath_arm64)
    utl.CopyFile("{}/build/amd64/wstunnel.exe".format(os.getcwd()), "{}/{}".format(outpath_amd64, exe_name))
    utl.CopyFile("{}/build/arm64/wstunnel.exe".format(os.getcwd()), "{}/{}".format(outpath_arm64, exe_name))
    msg.Print("{} installed in {} and {}".format(DEP_TITLE, outpath_amd64, outpath_arm64))


def BuildDependencyMacOS(product_name):
    exe_base = "{}wstunnel".format(product_name)
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64", "GOOS": "darwin"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", exe_base + "-arm64"), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "amd64", "GOOS": "darwin"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", exe_base + "-amd64"), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)


def BuildDependencyLinux(product_name):
    exe_name = "{}wstunnel".format(product_name)
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64" if platform.machine() in ("arm64", "aarch64") else "amd64", "GOOS": "linux"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", exe_name), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)


def InstallDependency():
    product_name = iutl.ParseProductArg()
    dep_file_mask = ["{}wstunnel.exe".format(product_name), "{}wstunnel".format(product_name)]
    # Load environment.
    msg.HeadPrint("Loading: \"{}\"".format(CONFIG_NAME))
    configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
    if not configdata:
        raise iutl.InstallError("Failed to get config data.")
    iutl.SetupEnvironment(configdata)
    dep_name = DEP_TITLE.lower()
    dep_version_var = "VERSION_" + DEP_TITLE.upper().replace("-", "")
    dep_version_str = os.environ.get(dep_version_var, None)
    if not dep_version_str:
        raise iutl.InstallError("{} not defined.".format(dep_version_var))
    # Prepare output.
    temp_dir = iutl.PrepareTempDirectory(dep_name)
    with utl.PushDir(temp_dir):
        iutl.RunCommand(["git", "clone", DEP_URL, "."])
        iutl.RunCommand(["git", "checkout", "tags/{}".format(dep_version_str)])
    # Build the dependency.
    msg.Info("Building: {}".format(DEP_TITLE))
    exe_base = "{}wstunnel".format(product_name)
    if utl.GetCurrentOS() == "win32":
        iutl.CopyCustomFiles(dep_name, temp_dir)
        template_in = os.path.join(TOOLS_DIR, "deps", "custom_wstunnel", "wstunnel.rc.in")
        configured_rc = os.path.join(temp_dir, "wstunnel.rc")
        iutl.ConfigureDependencyTemplate(product_name, template_in, configured_rc)
        with utl.PushDir(temp_dir):
            BuildDependencyWindows(product_name)
    elif utl.GetCurrentOS() == "macos":
        dep_buildroot_str = os.path.join(iutl.GetBuildLibsRoot(product_name), dep_name)
        outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
        utl.RemoveDirectory(outpath)
        with utl.PushDir(temp_dir):
            BuildDependencyMacOS(product_name)
        utl.CreateDirectory(outpath)
        with utl.PushDir(temp_dir):
            iutl.RunCommand([
                "lipo",
                "-create",
                os.path.join("build", exe_base + "-amd64"),
                os.path.join("build", exe_base + "-arm64"),
                "-output",
                os.path.join("build", exe_base)])
        iutl.InstallArtifacts(os.path.join(temp_dir, "build"), dep_file_mask, outpath, None)
    else:
        dep_buildroot_str = os.path.join(iutl.GetBuildLibsRoot(product_name), dep_name)
        outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
        utl.RemoveDirectory(outpath)
        with utl.PushDir(temp_dir):
            BuildDependencyLinux(product_name)
        iutl.InstallArtifacts(os.path.join(temp_dir, "build"), dep_file_mask, outpath, None)
    # Cleanup.
    msg.Print("Cleaning temporary directory...")
    utl.RemoveDirectory(temp_dir)


if __name__ == "__main__":
    start_time = time.time()
    current_os = utl.GetCurrentOS()
    is_arm64_build = "--arm64" in sys.argv
    if current_os not in DEP_OS_LIST:
        msg.Print("{} is not needed on {}, skipping.".format(DEP_TITLE, current_os))
        sys.exit(0)
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
