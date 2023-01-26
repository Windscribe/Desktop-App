#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: runs TestGui

import glob2
import os
import re
import sys
import time
import zipfile

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)
COMMON_DIR = os.path.join(ROOT_DIR, "common")
sys.path.insert(0, TOOLS_DIR)

import base.messages as msg
import base.utils as utl
import base.process as proc
import deps.installutils as iutl

BUILD_TITLE = "TestGui"
BUILD_CFGNAME = "build_test_gui.yml"
BUILD_OS_LIST = [ "win32", "macos"]

BUILD_APP_VERSION_STRING = ""
RUN_TEST_GUI_FILES = ""

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
def CopyFile(filename, srcdir, dstdir, strip_first_dir=False):
  parts = filename.split("->")
  srcfilename = parts[0].strip()
  dstfilename = srcfilename if len(parts) == 1 else parts[1].strip()
  msg.Print(dstfilename)
  srcfile = os.path.normpath(os.path.join(srcdir, srcfilename))
  dstfile = os.path.normpath(dstfilename)
  if strip_first_dir:
    dstfile = os.sep.join(dstfile.split(os.path.sep)[1:])
  dstfile = os.path.join(dstdir, dstfile)
  utl.CopyAllFiles(srcfile, dstfile) \
    if srcfilename.endswith(("\\", "/")) else utl.CopyFile(srcfile, dstfile)

# TODO: move into shared py file
def CopyFiles(title, filelist, srcdir, dstdir, strip_first_dir=False):
  msg.Info("Copying {} files...".format(title))
  for filename in filelist:
    CopyFile(filename, srcdir, dstdir, strip_first_dir)

def CopyTestGuiFiles(configdata, qt_root, msvc_root, crt_root, targetDir):
  if "qt_files" in configdata:
      CopyFiles("Qt", configdata["qt_files"], qt_root, targetDir, strip_first_dir=True)
  if "msvc_files" in configdata:
    CopyFiles("MSVC", configdata["msvc_files"], msvc_root, targetDir)
  # utl.CopyAllFiles(crt_root, targetDir) # api-ms-win-...dlls
  if "lib_files" in configdata:
    for k, v in configdata["lib_files"].items():
      lib_root = iutl.GetDependencyBuildRoot(k)
      if not lib_root:
        raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
      CopyFiles(k, v, lib_root, targetDir)


def RunTestGui():
  # Load config.
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, "{}".format(BUILD_CFGNAME)))
  if not configdata:
    raise iutl.InstallError("Failed to load config \"{}\".".format(BUILD_CFGNAME))
  current_os = utl.GetCurrentOS()
  qt_root = iutl.GetDependencyBuildRoot("qt")
  if not qt_root:
    raise iutl.InstallError("Qt is not installed.")
  buildenv = os.environ.copy()

  if current_os == "win32":
    buildenv.update(iutl.GetVisualStudioEnvironment())
    msvc_root = os.path.join(buildenv["VCTOOLSREDISTDIR"], "x86", "Microsoft.VC141.CRT")
    crt_root = "C:\\Program Files (x86)\\Windows Kits\\10\\Redist\\{}\\ucrt\\DLLS\\x86".format(
      buildenv["WINDOWSSDKVERSION"])
    if not os.path.exists(msvc_root):
      raise iutl.InstallError("MSVS installation not found.")
    if not os.path.exists(crt_root):
      raise iutl.InstallError("CRT files not found.")
  # Prep dirs, check for binary
  artifact_dir = os.path.join(ROOT_DIR, "test-exe")
  global RUN_TEST_GUI_FILES
  RUN_TEST_GUI_FILES = os.path.join(artifact_dir, "gui")
  test_gui_exe = os.path.join(RUN_TEST_GUI_FILES, configdata["test-gui"]["name"])
  if current_os == "win32":
    test_gui_exe += ".exe"
    CopyTestGuiFiles(configdata, qt_root, msvc_root, crt_root, RUN_TEST_GUI_FILES)
  if not os.path.exists(test_gui_exe):
  	raise iutl.InstallError("Could not find test gui executable.")
  # Create output file
  global BUILD_APP_VERSION_STRING
  BUILD_APP_VERSION_STRING = ExtractAppVersion()
  test_gui_output = os.path.join(RUN_TEST_GUI_FILES, "{}-{}.log".format(configdata["test-gui"]["name"], BUILD_APP_VERSION_STRING))
  utl.CreateFile(test_gui_output, True)
  # Run test
  proc.ExecuteWithRealtimeOutput([test_gui_exe], None, False, test_gui_output)
  msg.Print("Successful run of {}".format(configdata["test-gui"]["name"]))

if __name__ == "__main__":
  start_time = time.time()
  current_os = utl.GetCurrentOS()
  if current_os not in BUILD_OS_LIST:
    msg.Print("{} is not needed on {}, skipping.".format(DEP_TITLE, current_os))
    sys.exit(0)
  try:
    msg.Print("Running {}...".format(BUILD_TITLE))
    RunTestGui()
    exitcode = 0
  except iutl.InstallError as e:
    msg.Error(e)
    exitcode = e.exitcode
  except IOError as e:
    msg.Error(e)
    exitcode = 1
  except RuntimeError as e:
  	msg.Error(e)
  	exitcode = 1
  elapsed_time = time.time() - start_time
  if elapsed_time >= 60:
    msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time/60, elapsed_time%60))
  else:
    msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
  sys.exit(exitcode)