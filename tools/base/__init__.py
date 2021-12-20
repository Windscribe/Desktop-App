#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
import os

import colorama

# Initialize colorama module for Win32 ANSI color support.
if os.getenv("TERM") is None:
  colorama.init()
