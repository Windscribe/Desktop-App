#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2022, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs Google Protobuf.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "protobuf.yml")

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "protobuf"
DEP_URL = "https://github.com/protocolbuffers/protobuf/archive/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["bin/**", "include/**", "lib/**"]

CMAKE_BINARY = "C:\\Program Files\\CMake\\bin\\cmake.exe"


def BuildDependencyMSVC(zlib_root, outpath):
  global DEP_FILE_MASK
  dep_file_mask_original = DEP_FILE_MASK
  # Create an environment with VS vars.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  shared_config_flags = [
    "-G", "NMake Makefiles",
    "-DZLIB_LIBRARY={}".format(os.path.join(zlib_root, "lib", "zdll.lib")),
    "-DZLIB_INCLUDE_DIR={}".format(os.path.join(zlib_root, "include")),
    "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF",
    "-Dprotobuf_BUILD_TESTS=OFF",
  ]
  # Build and install all configurations.
  currend_wd = os.getcwd()
  DEP_FILE_MASK = []
  for config in ["Debug", "Release"]:
    msg.HeadPrint("Building: {} version".format(config))
    config_lowercase = config.lower()
    configpath = os.path.join(currend_wd, "cmake", "build", config_lowercase)
    utl.CreateDirectory(configpath)
    os.chdir(configpath)
    build_cmd = [CMAKE_BINARY, "-DCMAKE_BUILD_TYPE=" + config,
                  "-DCMAKE_INSTALL_PREFIX={}".format(os.path.join(outpath, config_lowercase))]
    build_cmd.extend(shared_config_flags)
    build_cmd.append("../..")
    iutl.RunCommand(build_cmd, env=buildenv, shell=True)
    iutl.RunCommand(["nmake", "install", "-s"], env=buildenv, shell=True)
    DEP_FILE_MASK.extend([(config_lowercase + "/" + m) for m in dep_file_mask_original])


def BuildDependencyGNU(outpath):
  buildenv = os.environ.copy()
  # Configure.
  iutl.RunCommand("./autogen.sh", env=buildenv)
  configure_cmd = ["./configure"]
  if utl.GetCurrentOS() == "macos":
    configure_cmd.append("CXXFLAGS=-arch x86_64 -arch arm64 -mmacosx-version-min=10.14 -std=c++11")
  configure_cmd.append("--prefix={}".format(outpath))
  iutl.RunCommand(configure_cmd, env=buildenv)
  # Build and install.
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv)
  iutl.RunCommand(["make", "install", "-s"], env=buildenv)
  # Patch rpath to allow protoc to run when protobuf.zip is extracted to any arbitrary folder
  if utl.GetCurrentOS() == "macos":
    msg.Warn("The rpath for bin/protoc needs to be updated")
  else:
    iutl.RunCommand(["patchelf", "--set-rpath", "$ORIGIN/../lib", "{}/bin/protoc".format(outpath)])
    iutl.RunCommand(["patchelf", "--set-rpath", "$ORIGIN/../lib", "{}/lib/libprotoc.so".format(outpath)])


def InstallDependency():
  # Load environment.
  msg.HeadPrint("Loading: \"{}\"".format(CONFIG_NAME))
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
  if not configdata:
    raise iutl.InstallError("Failed to get config data.")
  iutl.SetupEnvironment(configdata)
  dep_name = DEP_TITLE.lower()
  dep_version_var = "VERSION_" + filter(lambda ch: ch not in "-", DEP_TITLE.upper())
  dep_version_str = os.environ.get(dep_version_var, None)
  if not dep_version_str:
    raise iutl.InstallError("{} not defined.".format(dep_version_var))
  if utl.GetCurrentOS() == "win32":
    if not os.path.exists(CMAKE_BINARY):
      raise iutl.InstallError("CMake is not installed.")
    zlib_root = iutl.GetDependencyBuildRoot("zlib")
    if not zlib_root:
      raise iutl.InstallError("ZLib is not installed.")
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Download and unpack the archive.
  archivetitle = "{}-{}".format(dep_name, dep_version_str)
  archivename = "v" + dep_version_str + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.gz")
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(DEP_URL, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  # Clean the output folder to ensure no conflicts when we're updating to a newer protobuf version.
  utl.RemoveDirectory(outpath)
  with utl.PushDir(os.path.join(temp_dir, archivetitle)):
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(zlib_root, outpath)
    else:
      BuildDependencyGNU(outpath)
  # Copy the dependency to output directory and to a zip file, if needed.
  aflist = [outpath]
  if "-zip" in sys.argv:
    installzipname = os.path.join(os.path.dirname(outpath), "{}.zip".format(dep_name))
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
