#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
import os
import shutil
import stat


def MakeExecutable(filename):
  os_stat = os.stat(filename)
  os.chmod(filename,
           os_stat.st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
