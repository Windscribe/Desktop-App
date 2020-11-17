#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
from __future__ import print_function

import os
import subprocess
import sys


def Execute(command_and_args, is_silent=False):
  execute_result = 1
  if not is_silent:
    process_handle = subprocess.Popen(command_and_args)
    execute_result = process_handle.wait()
  else:
    with open(os.devnull, "w") as file_out:
      process_handle = subprocess.Popen(command_and_args, stdout=file_out)
      execute_result = process_handle.wait()
  if execute_result:
    raise RuntimeError(execute_result)


class PipeRuntimeError(Exception):
  def __init__(self, exec_result, line_count):
    super(PipeRuntimeError, self).__init__()
    self.exec_result = exec_result
    self.line_count = line_count
  exec_result = 0
  line_count = 0

# Don't use pacifier detector on processes that ensure that their output will
# fit current console's width. Otherwise, there will be problems with carriage
# return.
PACIFIER_STATUS = False
PACIFIER_LAST_SIZE = 0


def ExecuteInPipe(command_and_args, strip_quotes=False, with_pacifier=False):
  global PACIFIER_STATUS, PACIFIER_LAST_SIZE
  if PACIFIER_STATUS != with_pacifier:
    PACIFIER_STATUS = with_pacifier
    PACIFIER_LAST_SIZE = 0
  output_line_count = 0
  process_handle = subprocess.Popen(command_and_args,
                                    stderr=subprocess.STDOUT,
                                    stdout=subprocess.PIPE,
                                    universal_newlines=True)
  for line in iter(process_handle.stdout.readline, ""):
    output_line_count += 1
    if strip_quotes:
      line = line.strip("'")
      if line[-2:] == "'\n" or line[-2:] == "'\r":
        line = line[:-2] + line[-1:]
    if with_pacifier and line[0:1] == "[":
      line = line.rstrip()
      if PACIFIER_LAST_SIZE > 0:
        print("\r" + " " * PACIFIER_LAST_SIZE + "\r", end="")
      PACIFIER_LAST_SIZE = len( line )
    elif PACIFIER_LAST_SIZE > 0:
      print("")
    print(line, end="")
    sys.stdout.flush()
  process_handle.stdout.close()
  execute_result = process_handle.wait()
  if execute_result:
    raise PipeRuntimeError(execute_result, output_line_count)
  return output_line_count
