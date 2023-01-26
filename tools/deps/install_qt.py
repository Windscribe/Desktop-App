#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs Qt.
import json
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "qt.yml")

import base.messages as msg
import base.utils as utl
from . import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "Qt"
DEP_URL = "https://download.qt.io/archive/qt/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["bin/**", "include/**", "lib/**", "mkspecs/**", "phrasebooks/**", "plugins/**",
                  "translations/**"]

QT_SKIP_MODULES = ["qtdoc", "qt3d", "qtactiveqt", "qtcanvas3d", "qtcharts", "qtconnectivity",
  "qtdatavis3d", "qtdeclarative", "qtdoc", "qtgamepad", "qtgraphicaleffects", "qtlocation",
  "qtmultimedia", "qtnetworkauth", "qtpurchasing", "qtquickcontrols", "qtquickcontrols2",
  "qtremoteobjects", "qtscript", "qtscxml", "qtserialbus", "qtserialport", "qtspeech",
  "qtvirtualkeyboard", "qtwayland", "qtwebchannel", "qtwebengine", "qtwebglplugin", "qtwebsockets",
  "qtwebview"]

QT_SOURCE_CHANGES_JSON_PATH = "deps/custom_qt/source_changes.json"

def ReplaceSourceCode(qt_source_dir):
  
  f = open(os.path.join(TOOLS_DIR, os.path.relpath(QT_SOURCE_CHANGES_JSON_PATH)), 'r')
  source_changes = json.load(f)
  for change in source_changes:
    file_path = os.path.join(os.path.abspath(qt_source_dir), os.path.relpath(change["file"]))
    with open(file_path, "r") as f_in:
      contents = f_in.read().decode("utf8")
      if contents.find(change["old_code"]) != -1:
        contents = contents.replace(change["old_code"], change["new_code"])
        f_in.close()
        with open(file_path, "w") as f_out:
          f_out.write(contents.encode("utf8"))
          f_out.close()
          print(("Replaced source code according to {}".format(change["ref"])))
      else:
        sys.exit("Code {} doesn't exist in {}".format(change["old_code"], file_path))

  f.close()

def BuildDependencyMSVC(installpath, openssl_root, outpath):
  # Create an environment with VS vars.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  buildenv.update({ "CL" : "/MP" })
  # Configure.
  configure_cmd = \
    ["configure.bat", "-opensource", "-confirm-license", "-openssl", "-nomake", "examples"]
  configure_cmd.extend(["-I", "{}\include".format(openssl_root)])
  configure_cmd.extend(["-L", "{}\lib".format(openssl_root)])
  configure_cmd.extend(["-prefix", installpath])
  if QT_SKIP_MODULES:
    configure_cmd.extend(x for t in zip(["-skip"] * len(QT_SKIP_MODULES), QT_SKIP_MODULES) for x in t)
  iutl.RunCommand(configure_cmd, env=buildenv, shell=True)
  # Build and install.
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv, shell=True)
  iutl.RunCommand(["nmake", "install"], env=buildenv, shell=True)

def BuildDependencyGNU(installpath, openssl_root, outpath):
  # Create an environment.
  buildenv = os.environ.copy()
  buildenv.update({ "OPENSSL_LIBS" : "-L{}/lib -lssl -lcrypto".format(openssl_root) })
  # Configure.
  configure_cmd = \
    ["./configure", "-opensource", "-confirm-license", "-release", "-nomake", "examples"]
  configure_cmd.append("-openssl-linked")
  configure_cmd.append("-I{}/include".format(openssl_root))
  configure_cmd.extend(["-prefix", installpath])
  if utl.GetCurrentOS() == "linux":
      configure_cmd.append("-no-opengl")
      configure_cmd.append("-qt-libpng")
  if QT_SKIP_MODULES:
    configure_cmd.extend(x for t in zip(["-skip"] * len(QT_SKIP_MODULES), QT_SKIP_MODULES) for x in t)
  iutl.RunCommand(configure_cmd, env=buildenv)
  # Build and install.
  iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv)
  iutl.RunCommand(["make", "install", "-s"], env=buildenv)

def InstallDependency():
  # Load environment.
  msg.HeadPrint("Loading: \"{}\"".format(CONFIG_NAME))
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
  if not configdata:
    raise iutl.InstallError("Failed to get config data.")
  iutl.SetupEnvironment(configdata)
  dep_name = DEP_TITLE.lower()
  dep_version_var = "VERSION_" + [ch for ch in DEP_TITLE.upper() if ch not in "-"]
  dep_version_str = os.environ.get(dep_version_var, None)
  if not dep_version_str:
    raise iutl.InstallError("{} not defined.".format(dep_version_var))
  openssl_root = iutl.GetDependencyBuildRoot("openssl")
  if not openssl_root:
    raise iutl.InstallError("OpenSSL is not installed.")
  # Prepare output.
  temp_dir = iutl.PrepareTempDirectory(dep_name)
  # Download and unpack the archive.
  dep_version_major = ".".join(dep_version_str.split(".")[0:2])
  archivetitle = "{}-everywhere-src-{}".format(dep_name, dep_version_str)
  archivename = archivetitle + (".zip" if utl.GetCurrentOS() == "win32" else ".tar.xz")
  localfilename = os.path.join(temp_dir, archivename)
  msg.HeadPrint("Downloading: \"{}\"".format(archivename))
  iutl.DownloadFile("{}/{}/{}/single/{}".format(DEP_URL, dep_version_major, dep_version_str, archivename), localfilename)
  msg.HeadPrint("Extracting: \"{}\"".format(archivename))
  iutl.ExtractFile(localfilename)
  # Windows workaround for long paths problem.
  if utl.GetCurrentOS() == "win32":
    extracteddir = "q"
    utl.RenameDirectory(os.path.join(temp_dir, archivetitle), os.path.join(temp_dir, extracteddir))
  else:
    extracteddir = archivetitle
  # Replace source code if necessary
  ReplaceSourceCode(os.path.join(temp_dir, extracteddir))
  # Build the dependency.
  dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
  dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
  outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
  with utl.PushDir(os.path.join(temp_dir, extracteddir)):
    msg.HeadPrint("Building: \"{}\"".format(archivetitle))
    if utl.GetCurrentOS() == "win32":
      BuildDependencyMSVC(outpath, openssl_root, temp_dir)
    else:
      BuildDependencyGNU(outpath, openssl_root, temp_dir)
    # add qt.conf to override the built-in QT_INSTALL_PREFIX
    qt_conf_path = os.path.join(outpath, "bin", "qt.conf")
    utl.CreateFileWithContents(qt_conf_path, "[Paths]\nPrefix = ..", True)
  # Copy the dependency to a zip file, if needed.
  aflist = [outpath]
  msg.Print("Installing artifacts...")
  if "-zip" in sys.argv:
    dep_artifact_var = "ARTIFACT_" + DEP_TITLE.upper()
    dep_artifact_str = os.environ.get(dep_artifact_var, "{}.zip".format(dep_name))
    installzipname = os.path.join(os.path.dirname(outpath), dep_artifact_str)
    aflist.extend(iutl.InstallArtifacts(outpath, DEP_FILE_MASK, None, installzipname))
  for af in aflist:
    msg.HeadPrint("Ready: \"{}\"".format(af))
  # Cleanup.
  msg.Print("Cleaning temporary directory...")
  utl.RemoveDirectory(temp_dir)


if __name__ == "__main__":

  start_time = time.time()
  current_os = utl.GetCurrentOS()
  if current_os not in DEP_OS_LIST:
    msg.Print("{} is not needed on {}, skipping.".format(DEP_TITLE, current_os))
    sys.exit(0)
  try:
    msg.Print("Installing {}...".format(DEP_TITLE))
    InstallDependency()
    exitcode = 0
  except iutl.InstallError as e:
    msg.Error(e)
    exitcode = e.exitcode
  except IOError as e:
    msg.Error(e)
    exitcode = 1
  elapsed_time = time.time() - start_time
  if elapsed_time >= 60:
    msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time / 60, elapsed_time % 60))
  else:
    msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
  sys.exit(exitcode)
