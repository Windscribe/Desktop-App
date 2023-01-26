#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs jom (multithreaded nmake).
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "jom.yml")

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "Jom"
DEP_URL = "http://download.qt.io/official_releases/"
DEP_OS_LIST = ["win32"]
DEP_FILE_MASK = ["jom.exe"]


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
  archivename = "{}_{}.zip".format(dep_name, dep_version_str)
  webfilename = "{}{}/{}".format(DEP_URL, dep_name, archivename)
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile(webfilename, localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Copy the dependency to output directory and to a zip file, if needed.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  installzipname = None
  if "-zip" in sys.argv:
    dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
    dep_artifact_str = os.environ.get(dep_artifact_var, "{}.zip".format(dep_name))
    installzipname = os.path.join(os.path.dirname(outpath), dep_artifact_str)
  msg.Print("Installing artifacts...")
  aflist = iutl.InstallArtifacts(temp_dir, DEP_FILE_MASK, outpath, installzipname)
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
