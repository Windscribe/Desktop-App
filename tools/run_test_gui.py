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
BUILD_OS_LIST = [ "win32" ]

BUILD_APP_VERSION_STRING = ""
BUILD_TEST_GUI_FILES = ""

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
  version_string = "{:d}_{:02d}_build{:d}".format(values[0], values[1], values[2])https://gitlab.int.windscribe.com/ws/client/desktop/client-desktop/-/jobs/41065#L61
  if values[3]:
    version_string += "_beta"
  return version_string

def RunTestGui():
  # Load config.
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, "{}".format(BUILD_CFGNAME)))
  if not configdata:
    raise iutl.InstallError("Failed to load config \"{}\".".format(BUILD_CFGNAME))
  current_os = utl.GetCurrentOS()

  temp_dir = iutl.PrepareTempDirectory("tests", False)
  global BUILD_TEST_GUI_FILES
  BUILD_TEST_GUI_FILES = os.path.join(temp_dir, "release")

  test_gui_exe = os.path.join(BUILD_TEST_GUI_FILES, configdata["test-gui"]["target"])
  if not os.path.exists(test_gui_exe):
  	raise iutl.InstallError("Could not find test gui executable.")

  # global BUILD_APP_VERSION_STRING
  # BUILD_APP_VERSION_STRING = ExtractAppVersion()
  # test_gui_output = os.path.join(BUILD_TEST_GUI_FILES, "test-gui-{}.log".format(BUILD_APP_VERSION_STRING))
  # utl.CreateFile(test_gui_output, True)

  proc.Execute([test_gui_exe])
  msg.Print("Successful run of test-gui")


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