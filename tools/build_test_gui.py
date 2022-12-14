#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: builds test-gui

# TODO: add better file structure so is more expandable to additional test binaries
#       IE temp/tests/test-gui/release instead of temp/tests/release

# TODO: ideally this script should handle building all the test binaries
#       this should only be done if possible to succeed at building second script if first script fails
#       (assuming they are independent)

import os
import re
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)
COMMON_DIR = os.path.join(ROOT_DIR, "common")
sys.path.insert(0, TOOLS_DIR)

import base.messages as msg
import base.utils as utl
import deps.installutils as iutl

BUILD_TITLE = "TestGui"
BUILD_CFGNAME = "build_test_gui.yml"
BUILD_OS_LIST = [ "win32", "macos" ]

BUILD_APP_VERSION_STRING = ""
BUILD_QMAKE_EXE = ""
BUILD_TEST_GUI_FILES = ""
BUILD_TEST_GUI_BINARIES = ""

# TODO: move into shared py file
def ExtractAppVersion():
  version_file = os.path.join(COMMON_DIR, "version", "windscribe_version.h")
  values = [0]*4
  patterns = [
    re.compile("\\bWINDSCRIBE_MAJOR_VERSION\\s+(\\d+)"),
    re.compile("\\bWINDSCRIBE_MINOR_VERSION\\s+(\\d+)"),
    re.compile("\\bWINDSCRIBE_BUILD_VERSION\\s+(\\d+)"),
    re.compile("^#define\\s+WINDSCRIBE_IS_BETA")
  ]
  with open(version_file, "r") as f:
    for line in f:
      for i in range(len(patterns)):
        matched = patterns[i].search(line)
        if matched:
          values[i] = int(matched.group(1)) if matched.lastindex > 0 else 1
          break
  version_string = "{:d}_{:02d}_build{:d}".format(values[0], values[1], values[2])
  if values[3]:
    version_string += "_beta"
  return version_string

# TODO: move into shared py file
def GenerateProtobuf():
  proto_root = iutl.GetDependencyBuildRoot("protobuf")
  if not proto_root:
    raise iutl.InstallError("Protobuf is not installed.")
  msg.Info("Generating Protobuf...")
  proto_gen = os.path.join(COMMON_DIR, "ipc", "proto", "generate_proto")
  proto_gen = proto_gen + (".bat" if utl.GetCurrentOS() == "win32" else ".sh")
  iutl.RunCommand([proto_gen, os.path.join(proto_root, "release", "bin")], shell=True)

# TODO: move into shared file
def GetProjectFile(subdir_name, project_name):
  return os.path.normpath(os.path.join(
  	os.path.dirname(TOOLS_DIR), subdir_name, project_name))

def BuildTestGui():
  # Load config.
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, "{}".format(BUILD_CFGNAME)))
  if not configdata:
    raise iutl.InstallError("Failed to load config \"{}\".".format(BUILD_CFGNAME))
  current_os = utl.GetCurrentOS()
  # TODO: parse check for build_test_gui.yml?
  # Extract app version.
  global BUILD_APP_VERSION_STRING
  BUILD_APP_VERSION_STRING = ExtractAppVersion()
  msg.Info("App version extracted: \"{}\"".format(BUILD_APP_VERSION_STRING))
  # Get Qt directory.
  qt_root = iutl.GetDependencyBuildRoot("qt")
  if not qt_root:
    raise iutl.InstallError("Qt is not installed.")
  buildenv = os.environ.copy()
  # Do some preliminary VS checks on Windows.
  if current_os == "win32":
    buildenv.update(iutl.GetVisualStudioEnvironment())
    msvc_root = os.path.join(buildenv["VCTOOLSREDISTDIR"], "x86", "Microsoft.VC141.CRT")
    crt_root = "C:\\Program Files (x86)\\Windows Kits\\10\\Redist\\{}\\ucrt\\DLLS\\x86".format(
      buildenv["WINDOWSSDKVERSION"])
    if not os.path.exists(msvc_root):
      raise iutl.InstallError("MSVS installation not found.")
    if not os.path.exists(crt_root):
      raise iutl.InstallError("CRT files not found.")
  # Prepare output
  artifact_dir = os.path.join(ROOT_DIR, "test-exe")
  utl.RemoveDirectory(artifact_dir)
  temp_dir = iutl.PrepareTempDirectory("tests")
  global BUILD_TEST_GUI_FILES
  BUILD_TEST_GUI_FILES = os.path.join(temp_dir, "gui")
  global BUILD_TEST_GUI_BINARIES
  if current_os == "win32":
    BUILD_TEST_GUI_BINARIES = os.path.join(BUILD_TEST_GUI_FILES, "release")
  else:
    BUILD_TEST_GUI_BINARIES = BUILD_TEST_GUI_FILES
  utl.CreateDirectory(BUILD_TEST_GUI_FILES)
  # Prep build tools
  global BUILD_QMAKE_EXE
  BUILD_QMAKE_EXE = os.path.join(qt_root, "bin", "qmake")
  if current_os == "win32":
    BUILD_QMAKE_EXE += ".exe"
    buildenv.update({ "MAKEFLAGS" : "S" })
    buildenv.update(iutl.GetVisualStudioEnvironment())
    buildenv.update({ "CL" : "/MP" })
  # Generate protobuf
  GenerateProtobuf()
  old_cwd = os.getcwd()
  os.chdir(BUILD_TEST_GUI_FILES)
  # build test-gui # TODO: make this more generic to accomodate multiple test binaries?
  iswin = current_os == "win32"
  test_gui = configdata["test-gui"]
  c_subdir = test_gui["subdir"]
  c_project = test_gui["project"]
  build_cmd = [ BUILD_QMAKE_EXE, GetProjectFile(c_subdir, c_project), "CONFIG+=release" ]
  if iswin: 
    build_cmd.extend(["-spec", "win32-msvc"])
  elif current_os == "macos":
    build_cmd.extend(["-spec", "macx-clang", "CONFIG+=x86_64"])
  iutl.RunCommand(build_cmd, env=buildenv, shell=iswin)
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv, shell=iswin)

  # move to test-exe
  artifact_dir = os.path.join(ROOT_DIR, "test-exe")
  utl.RemoveDirectory(artifact_dir)
  archive_test_gui = os.path.join(artifact_dir, "gui")
  utl.CreateDirectory(archive_test_gui, True)
  gui_test_src = os.path.join(BUILD_TEST_GUI_BINARIES, configdata["test-gui"]["name"])
  gui_test_dest = os.path.join(archive_test_gui, configdata["test-gui"]["name"])
  if iswin:
    gui_test_src += ".exe"
    gui_test_dest += ".exe"
  utl.CopyFile(gui_test_src, gui_test_dest)
  # clean up temp
  utl.RemoveDirectory(BUILD_TEST_GUI_FILES)

if __name__ == "__main__":
  start_time = time.time()
  current_os = utl.GetCurrentOS()
  if current_os not in BUILD_OS_LIST:
    msg.Print("{} is not needed on {}, skipping.".format(DEP_TITLE, current_os))
    sys.exit(0)
  try:
    msg.Print("Building {}...".format(BUILD_TITLE))
    BuildTestGui()
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
