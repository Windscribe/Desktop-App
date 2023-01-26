#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs CUrl library.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "curl.yml")

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "CUrl"
DEP_URL = "https://curl.haxx.se/download/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["bin/**", "include/**", "lib/**"]


def BuildDependencyMSVC(openssl_root, zlib_root):
  # Create an environment with VS vars.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  # Build and install.
  os.chdir("winbuild")
  build_cmd = ["nmake", "/F", "Makefile.vc", "mode=dll", "MACHINE=x86", "WITH_SSL=dll",
               "WITH_ZLIB=dll"]
  build_cmd.append("SSL_PATH={}".format(openssl_root))
  build_cmd.append("ZLIB_PATH={}".format(zlib_root))
  iutl.RunCommand(build_cmd, env=buildenv, shell=True)


def BuildDependencyGNU(openssl_root, outpath):
  # Create an environment with CC flags.
  buildenv = os.environ.copy()
  if utl.GetCurrentOS() == "macos":
    buildenv.update({ "CC" : "cc -mmacosx-version-min=10.11" })
  # Configure.
  configure_cmd = ["./configure"]
  configure_cmd.append("--prefix={}".format(outpath))
  configure_cmd.append("--with-ssl={}".format(openssl_root))
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
  dep_version_var = "VERSION_" + [ch for ch in DEP_TITLE.upper() if ch not in "-"]
  dep_version_str = os.environ.get(dep_version_var, None)
  if not dep_version_str:
    raise iutl.InstallError("{} not defined.".format(dep_version_var))
  openssl_root = iutl.GetDependencyBuildRoot("openssl")
  if not openssl_root:
    raise iutl.InstallError("OpenSSL is not installed.")
  if utl.GetCurrentOS() == "win32":
    zlib_root = iutl.GetDependencyBuildRoot("zlib")
    if not zlib_root:
      raise iutl.InstallError("Zlib is not installed.")
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Download and unpack the archive.
  archivetitle = "{}-{}".format(dep_name, dep_version_str)
  archivename = archivetitle + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.gz")
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(DEP_URL, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  with utl.PushDir(os.path.join(temp_dir, archivetitle)):
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(openssl_root, zlib_root)
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
  if utl.GetCurrentOS() == "win32":
    artifacts_dir = os.path.join(temp_dir, archivetitle, "builds", "libcurl-vc-x86-release-dll-ssl-dll-zlib-dll-ipv6-sspi")
    install_dir = outpath
  aflist = iutl.InstallArtifacts(artifacts_dir, DEP_FILE_MASK, install_dir, installzipname)
  for af in aflist:
    msg.HeadPrint("Ready: \"{}\"".format(af))
  # Cleanup.
  msg.Print("Cleaning temporary directory...")
  utl.RemoveDirectory(temp_dir)


if __name__ == "__main__":
  start_time = time.time()
  current_os = utl.GetCurrentOS()
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
