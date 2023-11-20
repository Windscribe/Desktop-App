#!/usr/bin/env python
# ------------------------------------------------------------------------------
# ESET VPN Build System
# Copyright (c) 2020-2023, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs Wstunnel executables.
import os
import shutil
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
DEP_FILE_MASK = ["windscribewstunnel.exe", "windscribewstunnel"]


def BuildDependencyMSVC():
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64" if is_arm64_build else "amd64", "GOOS": "windows"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", "windscribewstunnel.exe"), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)


def BuildDependencyMacOS():
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64", "GOOS": "darwin"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", "windscribewstunnel-arm64"), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "amd64", "GOOS": "darwin"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", "windscribewstunnel-amd64"), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)


def BuildDependencyLinux():
    buildenv = os.environ.copy()
    buildenv.update({"GOARCH": "arm64" if is_arm64_build else "amd64", "GOOS": "linux"})
    iutl.RunCommand(["go", "build", "-o", os.path.join("build", "windscribewstunnel"), "-a", "-gcflags=all=-l -B", "-ldflags=-w -s"], env=buildenv)


def InstallDependency():
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
    with utl.PushDir(os.path.join(temp_dir)):
        iutl.RunCommand(["git", "clone", DEP_URL, "."])
        iutl.RunCommand(["git", "checkout", "tags/{}".format(dep_version_str)])
    dep_buildroot_str = os.path.join("build-libs-arm64" if is_arm64_build else "build-libs", dep_name)
    outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
    # Clean the output folder to ensure no conflicts when we're updating to a newer stunnel version.
    utl.RemoveDirectory(outpath)
    # Build the dependency.
    msg.Info("Building: {}".format(DEP_TITLE))
    if utl.GetCurrentOS() == "macos":
        with utl.PushDir(os.path.join(temp_dir)):
            BuildDependencyMacOS()
        utl.CreateDirectory(outpath)
        with utl.PushDir(temp_dir):
            iutl.RunCommand([
                "lipo",
                "-create",
                os.path.join("build", "windscribewstunnel-amd64"),
                os.path.join("build", "windscribewstunnel-arm64"),
                "-output",
                os.path.join("build", "windscribewstunnel")])
    elif utl.GetCurrentOS() == "win32":
        shutil.copyfile(os.path.join(TOOLS_DIR, "deps", "custom_wstunnel", "rsrc_windows_arm64.syso" if is_arm64_build else "rsrc_windows_amd64.syso"), os.path.join(temp_dir, "rsrc.syso"))
        with utl.PushDir(temp_dir):
            BuildDependencyMSVC()
    else:
        with utl.PushDir(temp_dir):
            BuildDependencyLinux()

    # Copy the dependency to output directory and to a zip file, if needed.
    installzipname = None
    if "-zip" in sys.argv:
        dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
        dep_artifact_str = os.environ.get(dep_artifact_var, "{}.zip".format(dep_name))
        installzipname = os.path.join(os.path.dirname(outpath), dep_artifact_str)
    msg.Print("Installing artifacts...")
    iutl.InstallArtifacts(os.path.join(temp_dir, "build"), DEP_FILE_MASK, outpath, installzipname)
    # Cleanup.
    msg.Print("Cleaning temporary directory...")
    utl.RemoveDirectory(temp_dir)
    msg.Print(f"{DEP_TITLE} v{dep_version_str} installed in {outpath}")


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
