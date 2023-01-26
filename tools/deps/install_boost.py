#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited.  All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs Boost.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "boost.yml")

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "Boost"
#DEP_URL = "https://dl.bintray.com/boostorg/release/"
DEP_URL = "https://boostorg.jfrog.io/artifactory/main/release/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["include/**", "lib/**"]

BOOST_WITH_MODULES = ["regex", "serialization", "thread", "filesystem"]


def BuildDependencyMSVC(installpath, outpath):
  # Create an environment with VS vars.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update({ "CL" : "/MP" })
  # Configure.
  iutl.RunCommand(["bootstrap.bat"], env=buildenv, shell=True)
  # Build and install.  Use tagged layout to get installpath folder structure similar to MacOS/Linux.
  b2cmd = [".\\b2", "install", "-q", "link=static", "--build-type=complete", "--abbreviate-paths", "--layout=tagged"]
  b2cmd.append("--prefix={}".format(installpath))
  if BOOST_WITH_MODULES:
    b2cmd.extend(["--with-" + m for m in BOOST_WITH_MODULES])
  iutl.RunCommand(b2cmd, env=buildenv, shell=True)


def BuildDependencyGNU(installpath, outpath):
  # Create an environment.
  buildenv = os.environ.copy()
  # Configure.
  bootstrap_args = ""
  b2_args = [] 
  if utl.GetCurrentOS() == "macos":
    bootstrap_args = "--with-toolset=clang" 
    b2_args = ["toolset=clang", "cflags=-mmacosx-version-min=10.11",
      "cxxflags=-mmacosx-version-min=10.11", "mflags=-mmacosx-version-min=10.11",
      "mmflags=-mmacosx-version-min=10.11", "linkflags=-mmacosx-version-min=10.11"]
  iutl.RunCommand(["sh", "bootstrap.sh", "--prefix={}".format(installpath), bootstrap_args])
  # Build and install.
  b2cmd = ["./b2", "-q", "link=static"]
  b2cmd.extend(b2_args)
  b2_install_cmd = ["./b2" , "install"]
  if BOOST_WITH_MODULES:
    module_args = ["--with-" + m for m in BOOST_WITH_MODULES]
    b2cmd.extend(module_args)
    b2_install_cmd.extend(module_args)
  iutl.RunCommand(b2cmd)
  iutl.RunCommand(b2_install_cmd)
  # Remove dylibs.
  if utl.GetCurrentOS() == "macos":
    utl.RemoveAllFiles(os.path.join(installpath, "lib"), "*.dylib")


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
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Download and unpack the archive.
  dep_version_us = dep_version_str.replace(".","_")
  archivetitle = "{}_{}".format(dep_name, dep_version_us)
  archivename = archivetitle + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.gz")
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}/{}/source/{}".format(DEP_URL, dep_version_str, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  # Clean the output folder to ensure no conflicts when we're updating to a newer boost version.
  utl.RemoveDirectory(outpath)
  with utl.PushDir(os.path.join(temp_dir, archivetitle)):
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(outpath, temp_dir)
    else:
      BuildDependencyGNU(outpath, temp_dir)
  # Copy the dependency to a zip file, if needed.
  aflist = [outpath]
  msg.Print("Installing artifacts...")
  if "-zip" in sys.argv:
    dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
    dep_artifact_str = os.environ.get(dep_artifact_var, "{}.zip".format(dep_name))
    installzipname = os.path.join(os.path.dirname(outpath), dep_artifact_str)
    aflist.extend(iutl.InstallArtifacts(outpath, DEP_FILE_MASK, None, installzipname))
  for af in aflist:
    msg.HeadPrint("Ready: \"{}\"".format(af))
  # Cleanup.
  if "--no-clean" not in sys.argv:
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
