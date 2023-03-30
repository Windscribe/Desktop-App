#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2023, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs WireGuard executables.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "wireguard.yml")

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "WireGuard"
DEP_URL_WIN = "https://git.zx2c4.com/wireguard-windows/snapshot/"
DEP_URL_GNU = "https://git.zx2c4.com/wireguard-go/snapshot/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["windscribewireguard*", "*.dll"]


def BuildDependencyMSVC(outpath):
  buildenv = os.environ.copy()
  with utl.PushDir(os.path.join(os.getcwd(), "embeddable-dll-service")):
    currend_wd = os.getcwd()
    iutl.RunCommand(["build.bat"], env=buildenv, shell=True)
    utl.CopyFile("{}/amd64/tunnel.dll".format(currend_wd),
                 "{}/tunnel.dll".format(outpath))
  utl.CopyFile("{}/.deps/wireguard-nt/bin/amd64/wireguard.dll".format(os.getcwd()),
               "{}/wireguard.dll".format(outpath))


def BuildDependencyMacOS(build_arch):
  currend_wd = os.getcwd()
  buildenv = os.environ.copy()
  buildenv.update({ "BINDIR" : "wireguard", "DESTDIR" : os.path.dirname(currend_wd) + os.sep })
  buildenv.update({ "GOOS" : "darwin", "GOARCH" : "{}".format(build_arch) })
  iutl.RunCommand(["make"], env=buildenv)


def BuildDependencyLinux(outpath):
  currend_wd = os.getcwd()
  buildenv = os.environ.copy()
  buildenv.update({ "BINDIR" : "wireguard", "DESTDIR" : os.path.dirname(currend_wd) + os.sep })
  if utl.GetCurrentOS() == "macos":
    buildenv.update({ "GOARCH" : "arm64", "GOOS" : "darwin" })
  # Build and install.
  iutl.RunCommand(["go", "get", "-u", "golang.org/x/sys"], env=buildenv)
  iutl.RunCommand(["make"], env=buildenv)
  iutl.RunCommand(["make", "install", "-s"], env=buildenv)
  utl.CopyFile("{}/wireguard-go".format(currend_wd), "{}/windscribewireguard".format(outpath))


def InstallDependency():
  # Load environment.
  c_ismac = utl.GetCurrentOS() == "macos"
  msg.HeadPrint("Loading: \"{}\"".format(CONFIG_NAME))
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
  if not configdata:
    raise iutl.InstallError("Failed to get config data.")
  iutl.SetupEnvironment(configdata)
  dep_name = DEP_TITLE.lower()
  dep_version_var = "VERSION_" + DEP_TITLE.upper().replace("-", "") + ("_WIN" if utl.GetCurrentOS() == "win32" else "_GNU")
  dep_version_str = os.environ.get(dep_version_var, None)
  if not dep_version_str:
    raise iutl.InstallError("{} not defined.".format(dep_version_var))
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Download and unpack the archive.
  if utl.GetCurrentOS() == "win32":
    archivetitle = "wireguard-windows-{}".format(dep_version_str)
    download_url = DEP_URL_WIN;
  else:
    archivetitle = "wireguard-go-{}".format(dep_version_str)
    download_url = DEP_URL_GNU;
  archivename = archivetitle + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.xz")
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(download_url, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Copy modified files (Windows only).
  if utl.GetCurrentOS() == "win32":
    iutl.CopyCustomFiles(dep_name,os.path.join(temp_dir, archivetitle))
  if c_ismac:
    # Need to configure and build wireguard-go for each target architecture in its own folder.
    with utl.PushDir(temp_dir):
      iutl.RunCommand(["mv", archivetitle, archivetitle + "-arm64"])
      iutl.RunCommand(["cp", "-r", archivetitle + "-arm64", archivetitle + "-amd64"])
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  # Clean the output folder to ensure no conflicts when we're updating to a newer wireguard version.
  utl.RemoveDirectory(outpath)
  if c_ismac:
    msg.Info("Building: {} for architecture arm64".format(archivetitle))
    wireguard_src_dir_arm = os.path.join(temp_dir, archivetitle + "-arm64")
    with utl.PushDir(wireguard_src_dir_arm):
      BuildDependencyMacOS("arm64")
    msg.Info("Building: {} for architecture amd64".format(archivetitle))
    wireguard_src_dir_intel = os.path.join(temp_dir, archivetitle + "-amd64")
    with utl.PushDir(wireguard_src_dir_intel):
      BuildDependencyMacOS("amd64")
    utl.CreateDirectory(outpath)
    with utl.PushDir(temp_dir):
      iutl.RunCommand(["lipo", "-create", archivetitle + "-arm64/wireguard-go", archivetitle + "-amd64/wireguard-go", "-output", outpath + "/windscribewireguard"])
  else:
    with utl.PushDir(os.path.join(temp_dir, archivetitle)):
      msg.HeadPrint("Building: \"{}\"".format(archivetitle))
      if utl.GetCurrentOS() == "win32":
        BuildDependencyMSVC(outpath)
      else:
        BuildDependencyLinux(outpath)
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
