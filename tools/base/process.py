#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------


import os
import subprocess
import sys


def Execute(command_and_args, is_silent=False, env=None):
  execute_result = 1
  if not is_silent:
    process_handle = subprocess.Popen(command_and_args, env=env)
    execute_result = process_handle.wait()
  else:
    with open(os.devnull, "w") as file_out:
      process_handle = subprocess.Popen(command_and_args, stdout=file_out)
      execute_result = process_handle.wait()
  if execute_result:
    raise RuntimeError(execute_result)


def ExecuteWithRealtimeOutput(command_and_args, env=None, shell=False, logOutput=os.devnull):
  process_handle = subprocess.Popen(command_and_args, stdout=subprocess.PIPE, env=env, shell=shell)
  with open(logOutput, "w") as file_out:
    while True:
      output = process_handle.stdout.readline()
      if not output and process_handle.poll() is not None:
        break
      if output:
        file_out.write(output)
        print(output.strip())
        sys.stdout.flush()
    execute_result = process_handle.poll()
    if execute_result:
      raise RuntimeError(execute_result)


class PipeRuntimeError(Exception):
  def __init__(self, exec_result, line_count):
    super(PipeRuntimeError, self).__init__()
    self.exec_result = exec_result
    self.line_count = line_count
  exec_result = 0
  line_count = 0


def ExecuteInPipe(command_and_args, strip_quotes=False, env=None):
  output_line_count = 0
  process_handle = subprocess.Popen(command_and_args,
                                    stderr=subprocess.STDOUT,
                                    stdout=subprocess.PIPE,
                                    universal_newlines=True,
                                    env=env)
  for line in iter(process_handle.stdout.readline, ""):
    output_line_count += 1
    if strip_quotes:
      line = line.strip("'")
      if line[-2:] == "'\n" or line[-2:] == "'\r":
        line = line[:-2] + line[-1:]
    print(line, end="")
    sys.stdout.flush()
  process_handle.stdout.close()
  execute_result = process_handle.wait()
  if execute_result:
    raise PipeRuntimeError(execute_result, output_line_count)
  return output_line_count


def ExecuteAndGetOutput(command_and_args, env=None, shell=False):
  process_handle = subprocess.Popen(command_and_args, stdout=subprocess.PIPE, env=env, shell=shell)
  return process_handle.communicate()[0].strip()
