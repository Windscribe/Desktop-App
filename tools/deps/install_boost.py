#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2022, Windscribe Limited.  All rights reserved.
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
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "Boost"
#DEP_URL = "https://dl.bintray.com/boostorg/release/"
DEP_URL = "https://boostorg.jfrog.io/artifactory/main/release/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["include/**", "lib/**"]

BOOST_WITH_MODULES = ["filesystem", "regex", "serialization", "thread"]
BOOST_LIBS_CREATED = ["atomic", "filesystem", "regex", "serialization", "thread", "wserialization"]


def BuildDependencyMSVC(installpath):
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


def BuildDependencyMacOS(installpath, build_arch, install_dep):
  arch_type = ""
  if build_arch == "arm64":
    arch_type = "arm"
  else:
    arch_type = "x86"
  # Configure.
  iutl.RunCommand(["sh", "bootstrap.sh", "--prefix={}".format(installpath), "--with-toolset=clang",
    "cxxflags=\"-arch {}\"".format(build_arch), "cflags=\"-arch {}\"".format(build_arch), "linkflags=\"-arch {}\"".format(build_arch)])
  b2cmd = ["./b2", "-q", "link=static", "toolset=clang-darwin", "target-os=darwin", "architecture=" + arch_type,
    "cflags=-mmacosx-version-min=10.14 -arch {}".format(build_arch), "cxxflags=-mmacosx-version-min=10.14 -arch {}".format(build_arch),
    "mflags=-mmacosx-version-min=10.14 -arch {}".format(build_arch), "mmflags=-mmacosx-version-min=10.14 -arch {}".format(build_arch),
    "linkflags=-mmacosx-version-min=10.14 -arch {}".format(build_arch)]
  b2_install_cmd = ["./b2" , "install"]
  if BOOST_WITH_MODULES:
    module_args = ["--with-" + m for m in BOOST_WITH_MODULES]
    b2cmd.extend(module_args)
    b2_install_cmd.extend(module_args)
  iutl.RunCommand(b2cmd)
  if install_dep:
    iutl.RunCommand(b2_install_cmd)
    # Remove dylibs so they're not included in the zip.  We currently statically link to boost.
    utl.RemoveAllFiles(os.path.join(installpath, "lib"), "*.dylib")


def BuildDependencyLinux(installpath):
  # Configure.
  iutl.RunCommand(["sh", "bootstrap.sh", "--prefix={}".format(installpath)])
  # Build and install.
  b2cmd = ["./b2", "-q", "link=static"]
  b2_install_cmd = ["./b2" , "install"]
  if BOOST_WITH_MODULES:
    module_args = ["--with-" + m for m in BOOST_WITH_MODULES]
    b2cmd.extend(module_args)
    b2_install_cmd.extend(module_args)
  iutl.RunCommand(b2cmd)
  iutl.RunCommand(b2_install_cmd)


def InstallDependency():
  # Load environment.
  c_ismac = utl.GetCurrentOS() == "macos"
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
  if c_ismac:
    # Need to configure and build boost for each target architecture in its own folder.
    with utl.PushDir(temp_dir):
      iutl.RunCommand(["mv", archivetitle, archivetitle + "-arm64"])
      iutl.RunCommand(["cp", "-r", archivetitle + "-arm64", archivetitle + "-x86_64"])
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  # Clean the output folder to ensure no conflicts when we're updating to a newer boost version.
  utl.RemoveDirectory(outpath)
  if c_ismac:
    msg.Info("Building: {} for architecture arm64".format(archivetitle))
    boost_src_dir_arm = os.path.join(temp_dir, archivetitle + "-arm64")
    with utl.PushDir(boost_src_dir_arm):
      BuildDependencyMacOS(outpath, "arm64", False)
    msg.Info("Building: {} for architecture x86_64".format(archivetitle))
    boost_src_dir_intel = os.path.join(temp_dir, archivetitle + "-x86_64")
    with utl.PushDir(boost_src_dir_intel):
      BuildDependencyMacOS(outpath, "x86_64", True)
    with utl.PushDir(temp_dir):
      msg.Info("Creating macOS universal libs...")
      for lib_name in BOOST_LIBS_CREATED:
        iutl.RunCommand(["lipo", "-create", "{}-arm64/stage/lib/libboost_{}.a".format(archivetitle, lib_name),
        "{}-x86_64/stage/lib/libboost_{}.a".format(archivetitle, lib_name), "-output", "{}/lib/libboost_{}.a".format(outpath, lib_name)])
  else:
    with utl.PushDir(os.path.join(temp_dir, archivetitle)):
      msg.HeadPrint("Building: \"{}\"".format(archivetitle))
      if utl.GetCurrentOS() == "win32":
        BuildDependencyMSVC(outpath)
      else:
        BuildDependencyLinux(outpath)
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
