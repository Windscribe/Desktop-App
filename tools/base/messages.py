#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------


import sys

import colorama

HR_LINE = "-" * 64
NO_BANNER = False


def Error(message, **kwargs):
  print(colorama.Fore.RED + colorama.Style.BRIGHT + "ERROR: " + \
        colorama.Style.RESET_ALL + str( message ), **kwargs)
  sys.stdout.flush()


def Warn(message, **kwargs):
  print(colorama.Fore.YELLOW + colorama.Style.BRIGHT + "WARNING: " + \
        colorama.Style.RESET_ALL + str( message ), **kwargs)
  sys.stdout.flush()


def Info(message, **kwargs):
  print(colorama.Fore.GREEN + colorama.Style.BRIGHT + "INFO: " + \
        colorama.Style.RESET_ALL + str( message ), **kwargs)
  sys.stdout.flush()


def Print(message, **kwargs):
  print(message, **kwargs)
  sys.stdout.flush()


def Verbose(message, **kwargs):
  if "-v" in sys.argv:
    print(message, **kwargs)
    sys.stdout.flush()


def HeadPrint(message, **kwargs):
  first_space = message.find(":")
  the_rest = message
  if first_space >= 0:
    print(colorama.Style.BRIGHT + message[:first_space] + \
          colorama.Style.RESET_ALL, end='')
    the_rest = message[first_space:]
  print(the_rest, **kwargs)
  sys.stdout.flush()
