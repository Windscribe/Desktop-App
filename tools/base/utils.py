#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
import collections
import glob2
import platform
import os
import shutil
import stat
import time

import yaml
from . import messages as msg

_OS_CURRENT_NAME = None
_OS_RETRY_COUNT = 20

CURRENT_OS_MAC = "macos"
CURRENT_OS_LINUX = "linux"
CURRENT_OS_WIN = "win32"

def GetCurrentOS():
  global _OS_CURRENT_NAME
  if _OS_CURRENT_NAME is None:
    _OS_CURRENT_NAME = {
      "Windows": "win32", "Darwin": "macos", "Linux": "linux"
    }.get(platform.system(), "unknown")
    msg.Verbose("Detected OS: \"{}\"".format(_OS_CURRENT_NAME))
  return _OS_CURRENT_NAME


def LoadConfig(filename, loader=yaml.Loader):
  configdata = None
  msg.Verbose("LoadConfig: \"{}\"".format(filename))
  try:
    with open(filename) as stream:
      class OrderedLoader(loader):
        pass
      def ConstructMapping(loader, node):
        loader.flatten_mapping(node)
        return collections.OrderedDict(loader.construct_pairs(node))
      OrderedLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, ConstructMapping)
      configdata = yaml.load(stream, OrderedLoader)
  except IOError as e:
    msg.Verbose(e)
  return configdata


def MakeExecutable(filename):
  msg.Verbose("MakeExecutable: \"{}\"".format(filename))
  os_stat = os.stat(filename)
  os.chmod(filename,
           os_stat.st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def MakeUnicodePath(filename):
  fullpath = os.path.abspath("\\\\?\\" + filename) if GetCurrentOS() == "win32" else filename
  return str(fullpath, 'utf-8')


def remove_readonly_handler(func, path, excinfo):
  os.chmod(path, stat.S_IWRITE)
  func(path)

def RemoveDirectory(directory):
  msg.Verbose("RemoveDirectory: \"{}\"".format(directory))
  udirectory = MakeUnicodePath(directory)
  if not os.path.exists(udirectory):
    msg.Verbose("Directory doesn't exist")
    return True
  last_error = None
  for _ in range(_OS_RETRY_COUNT):
    try:
      # shutil.rmtree cannot remove read-only files/folders on Windows.  An 'access denied'
      # error occurs when it tries to do so.
      shutil.rmtree(udirectory, onerror=remove_readonly_handler)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't remove directory \"{}\"!\nLast error: {}".format(directory, last_error))


def CreateDirectory(directory, force_recreate=False):
  msg.Verbose("CreateDirectory: \"{}\"".format(directory))
  udirectory = MakeUnicodePath(directory)
  if os.path.exists(udirectory):
    if force_recreate:
      RemoveDirectory(directory)
    else:
      msg.Verbose("Directory already exists")
      return True
  for _ in range(_OS_RETRY_COUNT):
    try:
      os.makedirs(udirectory)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't create directory \"{}\"!\nLast error: {}".format(directory, last_error))

def CreateFile(filename, force_recreate=False):
  msg.Verbose("CreateFile: \"{}\"".format(filename))
  ufilename = MakeUnicodePath(filename)
  if os.path.exists(ufilename):
    if (force_recreate):
      RemoveFile(filename)
    else:
      msg.Verbose("File already exists")
      return False
  for _ in range(_OS_RETRY_COUNT):
    try:
      f = open(ufilename, "w")
      f.close()
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't create file \"{}\"!\nLast error: {}".format(filename, last_error))

def CreateFileWithContents(filename, text="", force_recreate=False):
  if CreateFile(filename, force_recreate):
    with open(filename, "wb") as file:
      file.write(text)
    return True
  raise IOError("Can't create file \"{}\"!\nLast error: {}".format(filename, last_error))

def RenameDirectory(srcname, dstname):
  msg.Verbose("RenameDirectory: \"{}\" to \"{}\"".format(srcname, dstname))
  usrcname = MakeUnicodePath(srcname)
  if not os.path.exists(usrcname):
    msg.Verbose("Directory doesn't exist")
    return True
  udstname = MakeUnicodePath(dstname)
  if os.path.exists(udstname):
    RemoveDirectory(dstname)
  last_error = None
  for _ in range(_OS_RETRY_COUNT):
    try:
      os.rename(usrcname, udstname)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't rename directory \"{}\" to \"{}\"!\nLast error: {}".format( \
    srcname, dstname, last_error))


def RemoveFile(filename):
  msg.Verbose("RemoveFile: \"{}\"".format(filename))
  ufilename = MakeUnicodePath(filename)
  if not os.path.exists(ufilename):
    msg.Verbose("File doesn't exist")
    return True
  last_error = None
  for _ in range(_OS_RETRY_COUNT):
    try:
      os.remove(ufilename)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't remove file \"{}\"!\nLast error: {}".format(filename, last_error))


def RenameFile(srcname, dstname):
  msg.Verbose("RenameFile: \"{}\" to \"{}\"".format(srcname, dstname))
  usrcname = MakeUnicodePath(srcname)
  if not os.path.exists(usrcname):
    msg.Verbose("File doesn't exist")
    return True
  udstname = MakeUnicodePath(dstname)
  if os.path.exists(udstname):
    RemoveFile(dstname)
  last_error = None
  for _ in range(_OS_RETRY_COUNT):
    try:
      os.rename(usrcname, udstname)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't rename file \"{}\" to \"{}\"!\nLast error: {}".format( \
    srcname, dstname, last_error))


def RemoveAllFiles(sourcedir, filemask):
  fullmask = os.path.join(sourcedir, filemask)
  msg.Verbose("RemoveAllFiles: \"{}\"".format(fullmask))
  for filename in glob2.glob(fullmask):
    if not os.path.isdir(filename):
      RemoveFile(filename)


def CopyFile(sourcefilename, destfilename, copymode=True):
  msg.Verbose("CopyFile: \"{}\" to \"{}\"".format(sourcefilename, destfilename))
  if not os.path.exists(sourcefilename):
    raise IOError("Can't copy file \"{}\": not found.".format(sourcefilename))
  usourcefilename = MakeUnicodePath(sourcefilename)
  udestfilename = MakeUnicodePath(destfilename)
  destdir = os.path.dirname(destfilename)
  if not os.path.exists(destdir):
    CreateDirectory(destdir)
  last_error = None
  for _ in range(_OS_RETRY_COUNT):
    try:
      shutil.copyfile(usourcefilename, udestfilename)
      if copymode:
        shutil.copymode(usourcefilename, udestfilename)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't copy file \"{}\" to \"{}\"!\nLast error: {}".format(
    sourcefilename, destfilename, last_error))


def CopyAllFiles(sourcedir, destdir, copymode=True):
  msg.Verbose("CopyAllFiles: \"{}\" to \"{}\"".format(sourcedir,destdir))
  if not os.path.exists(destdir):
    CreateDirectory(destdir)
  prefixlen = len(sourcedir)
  for dirpath, _, filenames in os.walk(sourcedir):
    for filename in filenames:
      srcfilename = os.path.join(dirpath, filename)
      dstfilename = os.path.join(destdir + dirpath[prefixlen:], filename)
      CopyFile(srcfilename, dstfilename, copymode)


def CopyMacBundle(sourcefilename, destfilename):
  msg.Verbose("CopyMacBundle: \"{}\" to \"{}\"".format(sourcefilename, destfilename))
  if not os.path.exists(sourcefilename):
    raise IOError("Can't copy bundle \"{}\": not found.".format(sourcefilename))
  destdir = os.path.dirname(destfilename)
  if not os.path.exists(destdir):
    CreateDirectory(destdir)
  last_error = None
  for _ in range(_OS_RETRY_COUNT):
    try:
      shutil.copytree(sourcefilename, destfilename, symlinks=True)
      return True
    except EnvironmentError as e:
      last_error = e
      time.sleep(1)
  raise IOError("Can't copy bundle \"{}\" to \"{}\"!\nLast error: {}".format(
    sourcefilename, destfilename, last_error))


class PushDir(object):
  def __init__(self, wd = None):
    self.wd = wd
    self.orig = os.getcwd()
  def __enter__(self):
    if self.wd: os.chdir(self.wd)
    return os.getcwd()
  def __exit__(self, type, value, traceback):
    os.chdir(self.orig)
