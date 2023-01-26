#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
import glob2
import multiprocessing
import os
import subprocess
import sys
import zipfile

import base.messages as msg
import base.process as proc
import base.utils as utl

REPOSITORY_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS_BIN_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),"bin")
TOOLS_VARS_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),"vars")
TEMP_DIR = os.path.join(REPOSITORY_ROOT, "temp")
CACHED_MAKE_COMMAND = None


class InstallError(Exception):
  def __init__(self, message, exitcode=1):
    super(InstallError, self).__init__(message)
    self.exitcode = exitcode


def SetupEnvironment(configdata):
  if configdata:
    msg.Verbose("SetupEnvironment:")
    if "variables" in configdata:
      for k, v in configdata["variables"].items():
        msg.Verbose("\"{}\" = \"{}\"".format(k,v))
        os.environ[k] = v


def PrepareTempDirectory(depname, doclean=True):
  temp_root_postfix = [ch for ch in depname.upper() if ch not in "-"]
  temp_root_var = None
  if utl.GetCurrentOS() == "win32":
    temp_root_var = "WINTEMPROOT_" + temp_root_postfix
    if not temp_root_var in os.environ:
      temp_root_var = None
  if not temp_root_var:
    temp_root_var = "TEMPROOT_" + temp_root_postfix
  temp_root_str = os.path.expanduser(os.path.normpath(os.environ[temp_root_var])) \
                  if temp_root_var in os.environ else TEMP_DIR
  temp_dir = os.path.join(temp_root_str, depname)
  if doclean:
    msg.HeadPrint("Cleaning: \"{}{}{}\"...".format(temp_root_str, os.sep, depname))
  utl.CreateDirectory(temp_dir, doclean)
  return temp_dir


def GetDependencyBuildRoot(depname):
  configdata = utl.LoadConfig(os.path.join(TOOLS_VARS_DIR, "{}.yml".format(depname)))
  buildroot_var = "BUILDROOT_" + depname.upper()
  buildroot_str = None
  if "variables" in configdata:
    for k, v in configdata["variables"].items():
      if k == buildroot_var:
          buildroot_str = os.path.normpath(os.path.join(REPOSITORY_ROOT, v))
          if not os.path.exists(buildroot_str):
            buildroot_str = None
          break
  return buildroot_str


def GetMakeBuildCommand():
  global CACHED_MAKE_COMMAND
  if CACHED_MAKE_COMMAND is None:
    if utl.GetCurrentOS() == "win32":
      buildroot_jom = GetDependencyBuildRoot("jom")
      if buildroot_jom:
        jom_exe_name = os.path.join(REPOSITORY_ROOT, buildroot_jom, "jom.exe")
        if os.path.exists(jom_exe_name):
          num_cores = str(multiprocessing.cpu_count())
          msg.Verbose("Found JOM: \"{}\" ({} cpu cores)".format(jom_exe_name, num_cores))
          CACHED_MAKE_COMMAND = [jom_exe_name, "-j", num_cores]
      if not CACHED_MAKE_COMMAND:
        CACHED_MAKE_COMMAND = ["nmake", "/NOLOGO"]
    else:
      num_cores = str(multiprocessing.cpu_count())
      CACHED_MAKE_COMMAND = ["make", "-j{}".format(num_cores)]
  return CACHED_MAKE_COMMAND


def DownloadFile(webfilename, localfilename):
  msg.Verbose("Downloading: \"{}\" to \"{}\"".format(webfilename, localfilename))
  if os.path.exists(localfilename):
    utl.RemoveDirectory(localfilename)
  try:
    curl_exe = os.path.join(TOOLS_BIN_DIR, "curl.exe") if utl.GetCurrentOS() == "win321" else "curl"
    curl_cmd = [ curl_exe, webfilename, "-o", localfilename, "-kL" ]
    if "-v" not in sys.argv:
      curl_cmd.append( "-#" )
    proc.ExecuteWithRealtimeOutput(curl_cmd)
  except RuntimeError as e:
    raise InstallError(e)


def ExtractFile(localfilename, outputpath=None, deleteonsuccess=True):
  if not outputpath:
    outputpath = os.path.dirname(localfilename)
  msg.Verbose("ExtractFile: \"{}\" to \"{}\"".format(localfilename, outputpath))
  extracted = False
  localfilename2 = None
  if utl.GetCurrentOS() == "win32":
    unz_exe = os.path.join(TOOLS_BIN_DIR, "7z.exe")
    if localfilename.endswith((".gz",".xz")):
      unz_cmd = [ unz_exe, "e", localfilename, "-o{}".format(os.path.dirname(localfilename)),
                  "-y", "-bso0", "-bsp2" ]
      try:
        proc.ExecuteWithRealtimeOutput(unz_cmd)
      except RuntimeError as e:
        msg.Verbose(e)
        raise InstallError("Error extracting archive file.")
      localfilename2 = localfilename
      localfilename = localfilename[:-3]
    unz_cmd = [ unz_exe, "x", localfilename, "-o{}".format(outputpath), "-y", "-bso0", "-bsp2" ]
    try:
      proc.ExecuteWithRealtimeOutput(unz_cmd)
      extracted = True
    except RuntimeError as e:
      msg.Verbose(e)
      raise InstallError("Error extracting archive file.")
  elif localfilename.endswith(".zip"):
    try:
      zf = zipfile.ZipFile(localfilename, mode="r")
      zf.extractall(outputpath)
      zf.close()
      extracted = True
    except zipfile.BadZipfile as e:
      msg.Verbose(e)
      raise InstallError("Error extracting ZIP file.")
  else:
    tar_options = "xf"
    # only mac tar (BSD impl) is smart enough to determine .xz is not gzip format
    if localfilename.endswith(".gz"):
        tar_options += "z"
    tar_cmd = [ "tar", tar_options , localfilename, "-C", outputpath ]
    try:
      proc.ExecuteWithRealtimeOutput(tar_cmd)
      extracted = True
    except RuntimeError as e:
      msg.Verbose(e)
      raise InstallError("Error extracting archive file.")
  if extracted and deleteonsuccess:
    utl.RemoveFile(localfilename)
    utl.RemoveFile(os.path.join(os.path.dirname(localfilename),"pax_global_header"))
    if localfilename2:
      utl.RemoveFile(localfilename2)


def GetVisualStudioEnvironment(architecture="x86"):
  result = None
  for batfile in [
    "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\BuildTools\\VC\\Auxiliary\\Build\\vcvarsall.bat",
    "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\Auxiliary\\Build\\vcvarsall.bat"
  ]:
    if os.path.isfile(batfile):
      process = subprocess.Popen(
        "(\"{}\" {}>nul)&&\"{}\" -c \"import os; print(repr(os.environ))\"".format(
          batfile, architecture, sys.executable), stdout=subprocess.PIPE, shell=True)
      stdout, _ = process.communicate()
      exitcode = process.wait()
      if exitcode == 0:
        result = eval(stdout.decode('ascii').strip('environ'))
      break
  if not result:
    raise InstallError("Error setting up Visual Studio environment.")
  return result


def RunCommand(command_and_args, env=None, shell=False):
  msg.Verbose("RunCommand: \"{}\" with env = \"{}\"".format(str(command_and_args), str(env)))
  try:
    proc.ExecuteWithRealtimeOutput(command_and_args, env=env, shell=shell)
  except RuntimeError as e:
    msg.Verbose(e)
    raise InstallError("Command failed: \"{}\"".format(str(command_and_args)))


def CopyCustomFiles(projectname, directory):
  srcdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "custom_" + projectname)
  if os.path.exists(srcdir):
    utl.CopyAllFiles(srcdir, directory)


def InstallArtifacts(directory, filemasks, installpath, zipfilename):
  aflist = []
  msg.Verbose("InstallArtifacts: \"{}\"[{}] to \"{}\" (zip=\"{}\")".format(
    directory, str(filemasks), installpath, zipfilename))
  if not installpath and not zipfilename:
    msg.Verbose("Nothing to do!")
    return aflist
  if installpath:
    utl.CreateDirectory(installpath, True)
    aflist.append(installpath)
  if zipfilename:
    utl.RemoveFile(zipfilename)
    zf = zipfile.ZipFile(zipfilename, "w", zipfile.ZIP_DEFLATED)
    aflist.append(zipfilename)
  for mask in filemasks:
    if "*" in mask:
      fullmask = os.path.join(directory, mask)
      skiplen = len(directory) + 1
      for srcfilename in glob2.glob(fullmask):
        if os.path.isdir(srcfilename):
          continue
        srcfilenamepartial = srcfilename[skiplen:]
        if installpath:
          dstfilename = os.path.join(installpath, srcfilenamepartial)
          utl.CopyFile(srcfilename, dstfilename)
        if zipfilename:
          zf.write(utl.MakeUnicodePath(srcfilename), srcfilenamepartial)
    else:
      srcfilename = os.path.join(directory, mask)
      if not os.path.exists(srcfilename):
        continue
      if installpath:
        dstfilename = os.path.join(installpath, mask)
        utl.CopyFile(srcfilename, dstfilename)
      if zipfilename:
        zf.write(utl.MakeUnicodePath(srcfilename), mask)
  if zipfilename:
    zf.close()
  return aflist
