#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs CUrl library.
import os
import platform
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "curl.yml")

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

from pathlib import Path

# Dependency-specific settings.
DEP_TITLE = "CUrl"
DEP_URL = "https://github.com/sftcd/curl/archive/ecf5952a8b668d59f7da4d082c91d555f3689aec.zip"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["bin/**", "include/**", "lib/**"]


def BuildDependencyMSVC(openssl_root, zlib_root, outpath):
    # Create an environment with VS vars.
    buildenv = os.environ.copy()
    buildenv.update({"MAKEFLAGS": "S"})
    buildenv.update(iutl.GetVisualStudioEnvironment(is_arm64_build))
    # Build and install.
    current_wd = os.getcwd()
    buildpath = os.path.join(current_wd, "build")
    utl.CreateDirectory(buildpath)
    os.chdir(buildpath)

    # As of curl 8.4.0, the cmake portion of the build process does not like Windows backslashes.
    openssl_root = Path(openssl_root).as_posix()
    zlib_root = Path(zlib_root).as_posix()

    build_cmd = ["cmake.exe", "..", "-DCURL_USE_OPENSSL=ON", "-DUSE_ECH=ON", "-DUSE_HTTPSRR=ON"]
    if is_arm64_build:
        build_cmd.append("-A ARM64")
        build_cmd.append("-DZLIB_LIBRARY={}/lib/zlib.lib".format(zlib_root))
    else:
        build_cmd.append("-DZLIB_LIBRARY={}/lib/zdll.lib".format(zlib_root))
    build_cmd.append("-DOPENSSL_ROOT_DIR={}".format(openssl_root))
    build_cmd.append("-DZLIB_INCLUDE_DIR={}/include".format(zlib_root))

    iutl.RunCommand(build_cmd, env=buildenv, shell=True)
    build_cmd = ["cmake.exe", "--build", ".", "--config", "Release"]
    iutl.RunCommand(build_cmd, env=buildenv, shell=True)

    build_cmd = ["cmake.exe", "--install", ".", "--prefix", outpath]
    iutl.RunCommand(build_cmd, env=buildenv, shell=True)


def BuildDependencyGNU(openssl_root, outpath):

    # Create an environment with CC flags.
    buildenv = os.environ.copy()
    if utl.GetCurrentOS() == "macos":
        buildenv.update({"CC": "cc -mmacosx-version-min=10.14 -arch x86_64 -arch arm64"})
    if utl.GetCurrentOS() == "linux" and platform.processor() == "aarch64":
        buildenv.update({"LDFLAGS": "-Wl,-rpath,{}/lib".format(openssl_root)})

    set_execute_permissions_cmd = ["chmod", "+x", "buildconf"]
    iutl.RunCommand(set_execute_permissions_cmd, env=buildenv)

    buildconf_cmd = ["./buildconf"]
    iutl.RunCommand(buildconf_cmd, env=buildenv)

    # Configure.
    configure_cmd = ["./configure"]
    configure_cmd.append("--prefix={}".format(outpath))
    configure_cmd.append("--with-openssl={}".format(openssl_root))
    configure_cmd.append("--enable-ech")
    configure_cmd.append("--enable-httpsrr")
    configure_cmd.append("--without-brotli")
    configure_cmd.append("--without-zstd")
    iutl.RunCommand(configure_cmd, env=buildenv)
    # Build and install.
    iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv)
    iutl.RunCommand(["make", "install", "-s"], env=buildenv)


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
    openssl_root = iutl.GetDependencyBuildRoot("openssl_ech_draft")
    if not openssl_root:
        raise iutl.InstallError("OpenSSL ECH Draft is not installed.")
    if utl.GetCurrentOS() == "win32":
        zlib_root = iutl.GetDependencyBuildRoot("zlib")
        if not zlib_root:
            raise iutl.InstallError("Zlib is not installed.")
    # Prepare output.
    temp_dir = iutl.PrepareTempDirectory(dep_name)
    # Download and unpack the archive.
    archivetitle = "curl"
    archivename = DEP_URL

    localfilename = os.path.join(temp_dir, "{}{}".format(archivetitle, ".zip"))
    msg.HeadPrint("Downloading: \"{}\"".format(archivename))
    iutl.DownloadFile("{}".format(archivename), localfilename)
    msg.HeadPrint("Extracting: \"{}\"".format(archivename))
    iutl.ExtractFile(localfilename)
    os.rename(os.path.join(temp_dir, "curl-ecf5952a8b668d59f7da4d082c91d555f3689aec"), os.path.join(temp_dir, archivetitle))

    # Copy modified files.
    iutl.CopyCustomFiles(dep_name, os.path.join(temp_dir, archivetitle))

    # Build the dependency.
    dep_buildroot_str = os.path.join("build-libs-arm64" if is_arm64_build else "build-libs", dep_name)
    outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
    # Clean the output folder to ensure no conflicts when we're updating to a newer curl version.
    utl.RemoveDirectory(outpath)
    with utl.PushDir(os.path.join(temp_dir, archivetitle)):
        msg.HeadPrint("Building: \"{}\"".format(archivetitle))
        if utl.GetCurrentOS() == "win32":
            BuildDependencyMSVC(openssl_root, zlib_root, outpath)
        else:
            BuildDependencyGNU(openssl_root, outpath)
    # Copy the dependency to output directory and to a zip file, if needed.
    installzipname = None
    if "-zip" in sys.argv:
        dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
        dep_artifact_str = os.environ.get(dep_artifact_var, "{}.zip".format(dep_name))
        installzipname = os.path.join(os.path.dirname(outpath), dep_artifact_str)
    msg.Print("Installing artifacts...")
    artifacts_dir = outpath
    install_dir = None
    aflist = iutl.InstallArtifacts(artifacts_dir, DEP_FILE_MASK, install_dir, installzipname)
    for af in aflist:
        msg.HeadPrint("Ready: \"{}\"".format(af))
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
