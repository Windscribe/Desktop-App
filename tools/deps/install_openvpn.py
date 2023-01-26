#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs OpenVPN executable.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "openvpn.yml")

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
# NOTE: we use different sources for Windows and POSIX, because apparently the GitHub version is
# now Windows-only (no configure scripts), and Community version doesn't build on Windows/MSVC.
DEP_TITLE = "OpenVPN"
DEP_URL_WIN32 = "https://github.com/OpenVPN/openvpn/archive/"
DEP_URL_POSIX = "https://swupdate.openvpn.org/community/releases/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["openvpn.exe", "openvpn"]


def BuildDependencyMSVC(openssl_root, lzo_root, outpath):
  # Create an environment with VS vars.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  buildenv.update({ "OPENVPN_DEPROOT" : outpath, "OPENSSL_HOME" : openssl_root, "LZO_HOME" : lzo_root })
  # Build and install.
  iutl.RunCommand("msvc-build.bat", env=buildenv, shell=True)
  currend_wd = os.getcwd()
  utl.CopyFile("{}/Win32-Output/Release/openvpn.exe".format(currend_wd),
               "{}/openvpn.exe".format(outpath))


def BuildDependencyGNU(openssl_root, lzo_root, outpath):
  # Create an environment with CC flags.
  buildenv = os.environ.copy()
  buildenv.update({ "CFLAGS" : "-I{}/include -I{}/include".format(openssl_root, lzo_root) })
  buildenv.update({ "CPPFLAGS" : "-I{}/include -I{}/include".format(openssl_root, lzo_root) })
  buildenv.update({ "LDFLAGS" : "-L{}/lib -L{}/lib".format(openssl_root, lzo_root) })
  if utl.GetCurrentOS() == "macos":
    buildenv.update({ "CC" : "cc -mmacosx-version-min=10.11" })
  # Configure.
  configure_cmd = ["./configure", "--with-crypto-library=openssl"]
  configure_cmd.append("--prefix={}".format(outpath))
  iutl.RunCommand(configure_cmd, env=buildenv)
  # Build and install.
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv)
  iutl.RunCommand(["make", "install-exec", "-s"], env=buildenv)
  utl.CopyFile("{}/sbin/openvpn".format(outpath), "{}/openvpn".format(outpath))


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
  lzo_root = iutl.GetDependencyBuildRoot("lzo")
  if not lzo_root:
    raise iutl.InstallError("LZO is not installed.")
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Download and unpack the archive.
  archivetitle = "{}-{}".format(dep_name, dep_version_str)
  if utl.GetCurrentOS() == "win32":
    dep_url = DEP_URL_WIN32
    archivename = "v{}.tar.gz".format(dep_version_str)
  else:
    dep_url = DEP_URL_POSIX
    archivename = archivetitle + ".tar.gz"
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(dep_url, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Copy modified files (Windows only).
  if utl.GetCurrentOS() == "win32":
    iutl.CopyCustomFiles(dep_name,os.path.join(temp_dir, archivetitle))
  # Build the dependency.
  with utl.PushDir(os.path.join(temp_dir, archivetitle)):
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(openssl_root, lzo_root, temp_dir)
    else:
      BuildDependencyGNU(openssl_root, lzo_root, temp_dir)
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
