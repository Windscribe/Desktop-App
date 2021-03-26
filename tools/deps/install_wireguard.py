#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs WireGuard executables.
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "wireguard.yml")

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "WireGuard"
DEP_URL = "https://git.zx2c4.com/wireguard-go/snapshot/"
DEP_OS_LIST = [ "win32", "macos", "linux" ]
DEP_FILE_MASK = [ "windscribewireguard*" ]

WIREGUARD_CONFIGS = { "386" : "x86", "amd64" : "x64" }


def BuildDependencyMSVC(toolpath, outpath):
  currend_wd = os.getcwd()
  toolpaths = [ os.path.join(toolpath,"go","bin"), os.path.join(toolpath,"bin") ]
  if "PATH" in os.environ:
    toolpaths.append(os.environ["PATH"])
  buildenv = os.environ.copy()
  buildenv.update({ "PATH" : ";".join(toolpaths) })
  # Build and install.
  for config, postfix in WIREGUARD_CONFIGS.iteritems():
    buildenv.update({ "GOARCH" : config })
    iutl.RunCommand(["make"], env=buildenv, shell=True)
    utl.CopyFile("{}/wireguard-go".format(currend_wd),
                 "{}/windscribewireguard_{}.exe".format(outpath, postfix))
    utl.RemoveFile("{}/wireguard-go".format(currend_wd))


def BuildDependencyGNU(outpath):
  currend_wd = os.getcwd()
  # Create an environment with CC flags.
  buildenv = os.environ.copy()
  buildenv.update({ "BINDIR" : "wireguard", "DESTDIR" : os.path.dirname(currend_wd) + os.sep })
  # Build and install.
  iutl.RunCommand(["make"], env=buildenv)
  iutl.RunCommand(["make", "install", "-s"], env=buildenv)
  utl.CopyFile("{}/wireguard-go".format(currend_wd), "{}/windscribewireguard".format(outpath))


def InstallWindowsToolchain(outpath):
  # Download and unpack go.
  tool_version_var = "VERSION_WIN32_GO"
  tool_version_str = os.environ[tool_version_var] if tool_version_var in os.environ else None
  if not tool_version_str:
    raise iutl.InstallError("{} not defined.".format(tool_version_var))
  archivename = "go{}.windows-amd64.zip".format(tool_version_str)
  localfilename = os.path.join(outpath, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format("https://dl.google.com/go/", archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Download and unpack make.
  tool_version_var = "VERSION_WIN32_MAKE"
  tool_version_str = os.environ[tool_version_var] if tool_version_var in os.environ else None
  if not tool_version_str:
    raise iutl.InstallError("{} not defined.".format(tool_version_var))
  archivename = "make-{}-without-guile-w32-bin.zip".format(tool_version_str)
  localfilename = os.path.join(outpath, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}{}".format(
    "https://download.wireguard.com/windows-toolchain/distfiles/", archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)


def InstallDependency():
  # Load environment.
  msg.HeadPrint("Loading: \"{}\"".format(CONFIG_NAME))
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
  if not configdata:
    raise iutl.InstallError("Failed to get config data.")
  iutl.SetupEnvironment(configdata)
  dep_name = DEP_TITLE.lower()
  dep_version_var = "VERSION_" + filter(lambda ch: ch not in "-", DEP_TITLE.upper())
  dep_version_str = os.environ[dep_version_var] if dep_version_var in os.environ else None
  if not dep_version_str:
    raise iutl.InstallError("{} not defined.".format(dep_version_var))
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Install build tools, if any.
  if utl.GetCurrentOS() == "win32" == "win32":
    msg.Print("Installing Windows toolchain...")
    win32_tooldir = os.path.join(temp_dir, "deps")
    utl.CreateDirectory(win32_tooldir, True)
    InstallWindowsToolchain(win32_tooldir)
  # Download and unpack the archive.
  archivetitle = "wireguard-go-{}".format(dep_version_str)
  archivename = archivetitle + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.xz")
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
  dep_buildroot_str = os.environ[dep_buildroot_var] if dep_buildroot_var in os.environ else \
                      os.path.join("build-libs", dep_name)
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  old_cwd = os.getcwd()
  os.chdir(os.path.join(temp_dir, archivetitle))
  msg.HeadPrint("Building: \"{}\"".format(archivetitle))
  BuildDependencyMSVC(win32_tooldir, outpath) \
    if utl.GetCurrentOS() == "win32" else BuildDependencyGNU(outpath)
  os.chdir(old_cwd)
  # Copy the dependency to output directory and to a zip file, if needed.
  aflist = [outpath]
  if "-zip" in sys.argv:
    dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
    dep_artifact_str = os.environ[dep_artifact_var] if dep_artifact_var in os.environ else \
                       "{}.zip".format(dep_name)
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
    msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time/60, elapsed_time%60))
  else:
    msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
  sys.exit(exitcode)
