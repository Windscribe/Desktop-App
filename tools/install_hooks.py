#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs project's Git hooks to local .git directory.
import os
import shutil
import sys

import base.messages as msg
import base.utils as utl

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
HOOKS_DIR = os.path.join(TOOLS_DIR, "hooks")
GIT_HOOKS_DIR = os.path.join(os.path.dirname(TOOLS_DIR), ".git", "hooks")

HOOKS_LIST = [
  "commit-msg",
  "pre-commit",
  "prepare-commit-msg"
]


def InstallGitHooks():
  if not os.path.exists(HOOKS_DIR):
    msg.Error("Hooks directory doesn't exist: " + HOOKS_DIR)
    return 1
  for hook_name in HOOKS_LIST:
    msg.Print("Installing hook: " + hook_name)
    try:
      destination_file = os.path.join(GIT_HOOKS_DIR, hook_name)
      shutil.copyfile(os.path.join(HOOKS_DIR, hook_name), destination_file)
      utl.MakeExecutable(destination_file)
    except EnvironmentError as e:
      msg.Error(e)
      return 1
  msg.Print("All done.")
  return 0


if __name__ == "__main__":
  sys.exit(InstallGitHooks())
