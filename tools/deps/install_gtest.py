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

CONFIG_NAME = os.path.join("vars", "gtest.yml")

CMAKE_BINARY_WIN = "C:\\Program Files\\CMake\\bin\\cmake.exe"
CMAKE_BINARY_MAC = "/Applications/CMake.app/Contents/bin/cmake"
CMAKE_BINARY_LINUX = "cmake"

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "GTest"
DEP_URL = "https://github.com/google/googletest/archive/"
DEP_OS_LIST = [ "win32", "macos", "linux" ]
DEP_FILE_MASK = [ "lib/**", "include/**"]

def BuildDependencyMSVC(outpath):
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  # Build and install.
  msg.HeadPrint("Building: {}".format(DEP_TITLE))
  current_wd = os.getcwd()
  buildpath = os.path.join(current_wd, "build")
  utl.CreateDirectory(buildpath)
  os.chdir(buildpath)
  if utl.GetCurrentOS() == "win32":
    build_cmd = [ CMAKE_BINARY_WIN, "..", "-Dgtest_force_shared_crt=ON" ]
  else:
    build_cmd = [CMAKE_BINARY_MAC, "..", "-Dgtest_force_shared_crt=ON"]

  iutl.RunCommand(build_cmd, env=buildenv, shell=True)
  iutl.RunCommand(["msbuild", "googletest-distribution.sln", "/p:Configuration=Release", "/p:Platform=Win32"], env=buildenv, shell=True)
  # copy libs
  utl.CreateDirectory("{}/include".format(outpath), True)
  utl.CopyAllFiles("{}/googlemock/include".format(current_wd), "{}/include".format(outpath))
  utl.CopyAllFiles("{}/googletest/include".format(current_wd), "{}/include".format(outpath))
  utl.CreateDirectory("{}/lib".format(outpath), True)
  utl.CopyAllFiles("{}/lib/Release".format(buildpath), "{}/lib".format(outpath))


def BuildDependencyGNU(outpath):
  currend_wd = os.getcwd()
  # Create an environment with CC flags.
  buildenv = os.environ.copy()
  if utl.GetCurrentOS() == "macos":
    buildenv.update({ "CC" : "cc -mmacosx-version-min=10.11" })
  # Build and install.
  msg.HeadPrint("Building: {}".format(DEP_TITLE))
  current_wd = os.getcwd()
  buildpath = os.path.join(current_wd, "build")
  utl.CreateDirectory(buildpath)
  os.chdir(buildpath)

  if utl.GetCurrentOS() == "macos":
    cmake_cmd = [ CMAKE_BINARY_MAC, "..",
                  "-Dgtest_force_shared_crt=ON",
                  "-DCMAKE_INSTALL_PREFIX={}".format(outpath)]
  else:
    cmake_cmd = [ CMAKE_BINARY_LINUX, "..",
                  "-Dgtest_force_shared_crt=ON",
                  "-DCMAKE_INSTALL_PREFIX={}".format(outpath)]
  iutl.RunCommand(cmake_cmd, env=buildenv)
  iutl.RunCommand(["make"], env=buildenv)
  iutl.RunCommand(["make", "install"], env=buildenv)


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
  archivetitle = "release-{}".format(dep_version_str)
  archivename = archivetitle + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.gz")
  archivepath = os.path.join(temp_dir, archivename)
  localfilename = os.path.join(temp_dir, "googletest-" + archivetitle)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(DEP_URL, archivename), archivepath)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(archivepath)
  # Copy the dependency to output directory and to a zip file, if needed.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  with utl.PushDir(os.path.join(temp_dir, localfilename)):
    buildpath = os.path.join(localfilename, "build")
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(outpath)
    else:
      BuildDependencyGNU(outpath)
  # Copy the dependency to output directory and to a zip file, if needed.
  installzipname = None
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
