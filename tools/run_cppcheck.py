#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: runs cppcheck on the whole project or on certain files.
import distutils.spawn
import os
import re
import sys
import time

import base.messages as msg
import base.process as proc

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
CPPCHECK_BUILD_DIR = os.path.join(os.path.dirname(TOOLS_DIR), "build-cppcheck")
MAX_ARGS = 512  # Maximum number of arguments for a subprocess.
WARNINGS_EXIT_CODE = 42


def Chunks(args, chunk_size):
  """Yield successive chunks from args with chunk_size."""
  for i in range(0, len(args), chunk_size):
    yield args[i:i + chunk_size]


def RunCppCheck():
  if not distutils.spawn.find_executable("cppcheck"):
    msg.Error("CppCheck is not installed, aborting.")
    return 1
  cppcheck_result = 0
  old_cwd = os.getcwd()

  is_quiet_mode = "--pipe" in sys.argv
  src_path = os.path.dirname(TOOLS_DIR)
  src_path_offset = len(src_path) + 1
  src_path_re = re.compile(r'(backend|common|gui)[\/](.*)')
  # Get files from pipe.
  src_files = []
  input_files = input().split() if is_quiet_mode else sys.argv[1:]
  for filepath in input_files:
    if filepath.startswith(src_path):
      src_files.append(filepath[src_path_offset:])
    else:
      re_check = src_path_re.match(filepath)
      if re_check:
        src_files.append(re_check.group(1) + "/" + re_check.group(2))

  num_src_files = len(src_files)
  if num_src_files == 0:
    msg.Print("Building a list of source files...", end=" ")
    for (dirpath, _, filenames) in os.walk(src_path):
      checkdirpath = dirpath.replace("\\", "/").lower()
      if any(checkdirpath.endswith(subdir) for subdir in \
             ("debug", "release", "generated_proto", ".vs")):
        continue
      if any(subdir in checkdirpath for subdir in \
              ("common/prepare_build_environment/",
               "common/wintun/",
               "/release/",
               "/debug/",
               "/generated_proto/",
               "/build-",
               "/.vs/"
              )):
        continue
      for filename in filenames:
        if any(filename.endswith( ext ) for ext in (".c", ".cc", ".cpp", ".cxx")):
          src_files.append(dirpath[src_path_offset:] + os.sep + filename)
      num_src_files = len(src_files)
    msg.Print("%i files found." % num_src_files)

  if num_src_files == 0:
    msg.Print("Nothing to do.")
    return 0
  start_time = time.time()
  cppcheck_args = [
    "cppcheck",
    "-q",
    "--cppcheck-build-dir=\"" + CPPCHECK_BUILD_DIR + "\"",
    "--template='[{severity}::{id}] {file}({line}): {message}'",
    "-j 4",
    "--force",
    "--language=c++",
    "--std=c++11",
    "--library=boost",
    "--library=qt",
    "--library=windows",
    "--platform=win32W",
    "--enable=warning,style,performance,portability",
    "--inline-suppr",
    "--suppress=objectIndex",
    "--suppress=ignoredReturnValue",
    "--suppress=passedByValue",
    "--suppress=unusedFunction",
    "--suppress=unusedPrivateFunction",
    "--suppress=useStlAlgorithm",
    "--suppress=qSortCalled",
    "--suppress=unreadVariable",
    "--suppress=redundantInitialization",
    "--suppress=redundantAssignment",
    "--suppress=constParameter",
    "--suppress=knownConditionTrueFalse",
    "--error-exitcode={}".format(WARNINGS_EXIT_CODE)
  ]
  os.chdir(src_path)
  src_chunk_size = max(MAX_ARGS - len(cppcheck_args), 1)
  total_errors_found = 0
  if not is_quiet_mode:
    if src_chunk_size < num_src_files:
      msg.Print("Running CppCheck in batches.")
    msg.Print("CppCheck started, please wait...")
  for src_batch in Chunks(src_files, src_chunk_size):
    try:
      proc.ExecuteInPipe(cppcheck_args + src_batch, True)
    except proc.PipeRuntimeError as e:
      cppcheck_result = e.exec_result
      if cppcheck_result != WARNINGS_EXIT_CODE:
        break  # Internal CppCheck error.
      total_errors_found += e.line_count
  if cppcheck_result == WARNINGS_EXIT_CODE:
    msg.Error("CppCheck found %i error(s) in the code." % total_errors_found)
  elif cppcheck_result != 0:
    msg.Error("CppCheck failed (error code %i)." % cppcheck_result)
  os.chdir(old_cwd)
  if not is_quiet_mode:
    elapsed_time = time.time() - start_time
    if elapsed_time >= 60:
      msg.HeadPrint("All done: %i minutes %i seconds elapsed" % \
                    (elapsed_time / 60, elapsed_time % 60))
    else:
      msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
  return cppcheck_result


if __name__ == "__main__":
  sys.exit(RunCppCheck())
