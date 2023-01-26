#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs C-Ares library.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "cares.yml")

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "CAres"
DEP_URL = "https://c-ares.haxx.se/download/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = []  # filled out later.

CARES_CONFIGS = {
  "dll_x32" : ["x86", "dll"],
  "dll_x64" : ["x86_amd64", "dll"],
  "static_x32" : ["x86", "lib"],
  "static_x64" : ["x86_amd64", "lib"],
}


def BuildDependencyMSVC(outpath):
  global DEP_FILE_MASK
  for prefix, params in CARES_CONFIGS.items():
    msg.HeadPrint("Building: {} ({} {})".format(prefix, params[0], params[1]))
    # Create an environment with VS vars.
    buildenv = os.environ.copy()
    buildenv.update({ "MAKEFLAGS" : "S" })
    buildenv.update({ "INSTALL_DIR" : "{}/{}".format(outpath, prefix) })
    buildenv.update(iutl.GetVisualStudioEnvironment(params[0]))
    buildenv.update({ "CL" : "/D_WINSOCK_DEPRECATED_NO_WARNINGS" })
    # Build and install.
    for buildcfg in ["debug", "release"]:
      iutl.RunCommand(["nmake", "/NOLOGO", "/F", "Makefile.msvc", 
                       "CFG={}-{}".format(params[1], buildcfg), "c-ares"], env=buildenv, shell=True)
      iutl.RunCommand(["nmake", "/F", "Makefile.msvc", "CFG={}-{}".format(params[1], buildcfg),
                       "install"], env=buildenv, shell=True)
    iutl.RunCommand(["nmake", "/F", "Makefile.msvc", "clean"], env=buildenv, shell=True)
    DEP_FILE_MASK.append("{}/**".format(prefix))


def BuildDependencyGNU(outpath):
  global DEP_FILE_MASK
  # Create an environment with CC flags.
  buildenv = os.environ.copy()
  args = ""
  if utl.GetCurrentOS() == "macos":
    args = "-mmacosx-version-min=10.11"
  buildenv.update({ "CC" : "cc {}".format(args)})
  # Configure.
  configure_cmd = ["./configure"]
  configure_cmd.append("--prefix={}".format(outpath))
  iutl.RunCommand(configure_cmd, env=buildenv)
  # Build and install.
  iutl.RunCommand(["make"], env=buildenv)
  iutl.RunCommand(["make", "install", "-s"], env=buildenv)
  for prefix in ["include", "lib"]:
    DEP_FILE_MASK.append("{}/**".format(prefix))


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
  archivetitle = "c-ares-{}".format(dep_version_str)
  archivename = archivetitle + ".tar.gz"
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(DEP_URL, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Copy modified files (Windows only).
  if utl.GetCurrentOS() == "win32":
    iutl.CopyCustomFiles(dep_name,os.path.join(temp_dir, archivetitle))
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  with utl.PushDir(os.path.join(temp_dir, archivetitle)):
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(outpath)
    else:
      BuildDependencyGNU(outpath)
  # Copy the dependency to output directory and to a zip file, if needed.
  aflist = [outpath]
  if "-zip" in sys.argv:
    dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
    dep_artifact_str = os.environ.get(dep_artifact_var, "{}.zip".format(dep_name))
    installzipname = os.path.join(os.path.dirname(outpath), dep_artifact_str)
    msg.Print("Installing artifacts...")
    aflist = iutl.InstallArtifacts(outpath, DEP_FILE_MASK, None, installzipname)
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
