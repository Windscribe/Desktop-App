#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2022, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs OpenSSL library.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "openssl.yml")

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "OpenSSL"
DEP_URL = "https://www.openssl.org/source/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["bin/*.dll", "include/**", "lib/**"]


def BuildDependencyMSVC(outpath):
  # Create an environment with VS vars.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  # Configure.
  is_testing_ok = "-test" in sys.argv
  configure_cmd = ["perl.exe", "Configure", "VC-WIN64A", "no-asm", "-FS"]
  if not is_testing_ok:
    configure_cmd.extend(["no-unit-test", "no-tests"])
  configure_cmd.append("--prefix={}".format(outpath))
  iutl.RunCommand(configure_cmd, env=buildenv, shell=True)
  # Build and install.
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv, shell=True)
  if is_testing_ok:
    iutl.RunCommand(["nmake", "test"], env=buildenv, shell=True)
  iutl.RunCommand(["nmake", "install_dev"], env=buildenv, shell=True)


def BuildDependencyMacOS(outpath, build_arch, install_dep):
  buildenv = os.environ.copy()
  buildenv.update({ "CC" : "cc -mmacosx-version-min=10.14"})
  configure_cmd = ["./Configure", "darwin64-{}-cc".format(build_arch), "shared", "no-asm", "no-unit-test", "no-tests"]
  configure_cmd.append("--prefix={}".format(outpath))
  configure_cmd.append("--openssldir=/usr/local/ssl")
  iutl.RunCommand(configure_cmd, env=buildenv)
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv)
  if install_dep:
    iutl.RunCommand(["make", "install_dev", "-s"], env=buildenv)


def BuildDependencyLinux(outpath):
  buildenv = os.environ.copy()
  # Configure.
  is_testing_ok = "-test" in sys.argv
  configure_cmd = ["./config", "--shared", "no-asm"]
  if not is_testing_ok:
    configure_cmd.extend(["no-unit-test", "no-tests"])
  configure_cmd.append("--prefix={}".format(outpath))
  configure_cmd.append("--openssldir=/usr/local/ssl")
  iutl.RunCommand(configure_cmd, env=buildenv)
  # Build and install.
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv)
  if is_testing_ok:
    iutl.RunCommand(["make", "test", "-s"], env=buildenv)
  iutl.RunCommand(["make", "install_dev", "-s"], env=buildenv)


def InstallDependency():
  # Load environment.
  c_ismac = utl.GetCurrentOS() == "macos"
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
  # Download and unpack the archive.
  archivetitle = "{}-{}".format(dep_name, dep_version_str)
  archivename = archivetitle + ".tar.gz"
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(DEP_URL, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  if c_ismac:
    # Need to configure and build openssl for each target architecture in its own folder.
    with utl.PushDir(temp_dir):
      iutl.RunCommand(["mv", archivetitle, archivetitle + "-arm64"])
      iutl.RunCommand(["cp", "-r", archivetitle + "-arm64", archivetitle + "-x86_64"])
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  # Clean the output folder to ensure no conflicts when we're updating to a newer openssl version.
  utl.RemoveDirectory(outpath)
  if c_ismac:
    msg.Info("Building: {} for architecture arm64".format(archivetitle))
    openssl_src_dir_arm = os.path.join(temp_dir, archivetitle + "-arm64")
    with utl.PushDir(openssl_src_dir_arm):
      BuildDependencyMacOS(outpath, "arm64", False)
    msg.Info("Building: {} for architecture x86_64".format(archivetitle))
    openssl_src_dir_intel = os.path.join(temp_dir, archivetitle + "-x86_64")
    with utl.PushDir(openssl_src_dir_intel):
      BuildDependencyMacOS(outpath, "x86_64", True)
    with utl.PushDir(temp_dir):
      iutl.RunCommand(["lipo", "-create", archivetitle + "-arm64/libcrypto.a", archivetitle + "-x86_64/libcrypto.a", "-output", outpath + "/lib/libcrypto.a"])
      iutl.RunCommand(["lipo", "-create", archivetitle + "-arm64/libssl.a", archivetitle + "-x86_64/libssl.a", "-output", outpath + "/lib/libssl.a"])
      iutl.RunCommand(["lipo", "-create", archivetitle + "-arm64/libcrypto.1.1.dylib", archivetitle + "-x86_64/libcrypto.1.1.dylib", "-output", outpath + "/lib/libcrypto.1.1.dylib"])
      iutl.RunCommand(["lipo", "-create", archivetitle + "-arm64/libssl.1.1.dylib", archivetitle + "-x86_64/libssl.1.1.dylib", "-output", outpath + "/lib/libssl.1.1.dylib"])
  else:
    with utl.PushDir(os.path.join(temp_dir, archivetitle)):
      msg.Info("Building: \"{}\"".format(archivetitle))
      if utl.GetCurrentOS() == "win32":
        BuildDependencyMSVC(outpath)
      else:
        BuildDependencyLinux(outpath)
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
