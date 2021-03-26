#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: builds Windscribe.
import glob2
import os
import re
import sys
import time
import zipfile

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)
COMMON_DIR = os.path.join(ROOT_DIR, "common")
sys.path.insert(0, TOOLS_DIR)

import base.messages as msg
import base.process as proc
import base.utils as utl
import deps.installutils as iutl

# Windscribe settings.
BUILD_TITLE = "Windscribe"
BUILD_CFGNAME = "build_all.yml"
BUILD_OS_LIST = [ "win32", "macos" ]
BUILD_CERT_PASSWORD = "fBafQVi0RC4Ts4zMUFOE" # TODO: keep elsewhere!

BUILD_APP_VERSION_STRINGS = ()
BUILD_QMAKE_EXE = ""
BUILD_MACDEPLOY = ""
BUILD_INSTALLER_FILES = ""
BUILD_SYMBOL_FILES = ""


def ExtractAppVersion():
  version_file = os.path.join(COMMON_DIR, "version", "windscribe_version.h")
  values = [0]*4
  patterns = [
    re.compile("\\bWINDSCRIBE_MAJOR_VERSION\\s+(\\d+)"),
    re.compile("\\bWINDSCRIBE_MINOR_VERSION\\s+(\\d+)"),
    re.compile("\\bWINDSCRIBE_BUILD_VERSION\\s+(\\d+)"),
    re.compile("^#define\\s+WINDSCRIBE_IS_BETA")
  ]
  with open(version_file, "r") as f:
    for line in f:
      for i in range(len(patterns)):
        matched = patterns[i].search(line)
        if matched:
          values[i] = int(matched.group(1)) if matched.lastindex > 0 else 1
          break
  version_string_1 = "{:d}_{:02d}_build{:d}".format(values[0], values[1], values[2])
  version_string_2 = "{:d}.{:d}.{:d}".format(values[0], values[1], values[2])
  if values[3]:
    version_string_1 += "_beta"
  return (version_string_1, version_string_2)


def UpdateVersionInPlist(plistfilename):
  with open(plistfilename, "r") as file:
    filedata = file.read()
  filedata = re.sub("<key>CFBundleVersion</key>\n[^\n]+",
    "<key>CFBundleVersion</key>\n\t<string>{}</string>".format(BUILD_APP_VERSION_STRINGS[1]),
    filedata, flags = re.M)
  with open(plistfilename, "w") as file:
    file.write(filedata)


def GetProjectFile(subdir_name, project_name):
  return os.path.normpath(os.path.join(ROOT_DIR, subdir_name, project_name))


def GenerateProtobuf():
  proto_root = iutl.GetDependencyBuildRoot("protobuf")
  if not proto_root:
    raise iutl.InstallError("Protobuf is not installed.")
  msg.Info("Generating Protobuf...")
  proto_gen = os.path.join(COMMON_DIR, "ipc", "proto", "generate_proto")
  if utl.GetCurrentOS() == "win32":
    proto_gen = proto_gen + ".bat"
    iutl.RunCommand([proto_gen, os.path.join(proto_root, "release", "bin")], shell=True)
  else:
    proto_gen = proto_gen + ".sh"
    iutl.RunCommand([proto_gen, os.path.join(proto_root, "bin")], shell=True)


def BuildComponent(component, is_64bit, buildenv):
  # Collect settings.
  current_os = utl.GetCurrentOS()
  c_iswin = current_os == "win32"
  c_ismac = current_os == "macos"
  c_bits = "64" if (is_64bit or c_ismac) else "32"
  c_project = component["project"]
  c_subdir = component["subdir"]
  c_target = component["target"] if "target" in component else None
  if c_ismac and "macapp" in component:
    c_target = component["macapp"]
  # Strip "exe" if not on Windows.
  if not c_iswin and c_target.endswith(".exe"):
    c_target = c_target[:len(c_target)-4]
  # Setup a directory.
  msg.Info("Building {} ({}-bit)...".format(component["name"], c_bits))
  current_wd = os.getcwd()
  temp_wd = os.path.normpath(os.path.join(current_wd, c_subdir))
  utl.CreateDirectory(temp_wd)
  os.chdir(temp_wd)
  if c_project.endswith(".pro"):
    # Build Qt project.
    build_cmd = [ BUILD_QMAKE_EXE, GetProjectFile(c_subdir, c_project), "CONFIG+=release silent" ]
    if c_iswin:
      build_cmd.extend(["-spec", "win32-msvc"])
    iutl.RunCommand(build_cmd, env=buildenv, shell=c_iswin)
    iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv, shell=c_iswin)
    target_location = "release" if c_iswin else ""
    if c_ismac and "macapp" in component:
      deploy_cmd = [ BUILD_MACDEPLOY, component["macapp"]]
      if "plugins" in component and not component["plugins"]:
        deploy_cmd.append("-no-plugins")
      iutl.RunCommand(deploy_cmd, env=buildenv)
      UpdateVersionInPlist(os.path.join(temp_wd,component["macapp"],"Contents","Info.plist"))
  elif c_project.endswith(".vcxproj"):
    # Build MSVC project.
    conf = "Release_x64" if is_64bit else "Release"
    build_cmd = [
      "msbuild.exe", GetProjectFile(c_subdir, c_project),
      "/p:SolutionDir={}".format(ROOT_DIR + os.sep),
      "/p:OutDir={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
      "/p:IntDir={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
      "/p:IntermediateOutputPath={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
      "/p:Configuration={}".format(conf), "-nologo", "-verbosity:m" ]
    iutl.RunCommand(build_cmd, env=buildenv, shell=True)
    target_location = "release-{}".format(c_bits)
  elif c_project.endswith(".xcodeproj"):
    # Build Xcode project.
    build_cmd = [ "xcodebuild", "-scheme", component["scheme"], "-configuration", "Release",
                  "-quiet"]
    if "xcflags" in component:
      build_cmd.extend(map(lambda s : "\"" + s + "\"", component["xcflags"]))
    if "ExternalCompilerOptions" in buildenv:
      build_cmd.append("OTHER_CFLAGS=\"{}\"".format(buildenv["ExternalCompilerOptions"]))
    build_cmd.extend(["clean", "build"])
    os.chdir(os.path.join(ROOT_DIR, c_subdir))
    iutl.RunCommand(build_cmd, env=buildenv)
    if c_target:
      outdir = proc.ExecuteAndGetOutput(["xcodebuild -project {} -showBuildSettings | " \
                                        "grep -m 1 \"BUILT_PRODUCTS_DIR\" | " \
                                        "grep -oEi \"\/.*\"".format(c_project)], shell=True)
      if c_target.endswith(".app"):
        utl.CopyAllFiles(os.path.join(outdir, c_target), os.path.join(temp_wd, c_target))
      else:
        utl.CopyFile(os.path.join(outdir, c_target), os.path.join(temp_wd, c_target))
    os.chdir(temp_wd)
    target_location = ""
  else:
    raise iutl.InstallError("Unknown project type: \"{}\"!".format(c_project))
  # Copy output file.
  if c_target:
    srcfile = os.path.join(temp_wd, target_location, c_target)
    dstfile = BUILD_INSTALLER_FILES
    if "outdir" in component:
      dstfile = os.path.join(dstfile, component["outdir"])
    dstfile = os.path.join(dstfile, c_target)
    if c_target.endswith(".app"):
      utl.CopyAllFiles(srcfile, dstfile)
    else:
      utl.CopyFile(srcfile, dstfile)
  if BUILD_SYMBOL_FILES and "symbols" in component:
    srcfile = os.path.join(temp_wd, target_location, component["symbols"])
    dstfile = BUILD_SYMBOL_FILES
    if "outdir" in component:
      dstfile = os.path.join(dstfile, component["outdir"])
    dstfile = os.path.join(dstfile, component["symbols"])
    utl.CopyFile(srcfile, dstfile)
  os.chdir(current_wd)


def BuildComponents(configdata, targetlist, qt_root):
  # Setup globals.
  current_os = utl.GetCurrentOS()
  global BUILD_QMAKE_EXE, BUILD_MACDEPLOY
  BUILD_QMAKE_EXE = os.path.join(qt_root, "bin", "qmake")
  if current_os == "win32":
    BUILD_QMAKE_EXE += ".exe"
  BUILD_MACDEPLOY = os.path.join(qt_root, "bin", "macdeployqt")
  # Create an environment with compile-related vars.
  buildenv = os.environ.copy()
  if current_os == "win32":
    buildenv.update({ "MAKEFLAGS" : "S" })
    buildenv.update(iutl.GetVisualStudioEnvironment())
    buildenv.update({ "CL" : "/MP" })
    if "debug" in sys.argv:
      buildenv.update({ "ExternalCompilerOptions" : "/DSKIP_PID_CHECK" })
  else:
    if "debug" in sys.argv:
      buildenv.update({ "ExternalCompilerOptions" : "-DDISABLE_HELPER_SECURITY_CHECK=1" })
  # Build all components needed.
  has_64bit = False
  for target in targetlist:
    if not target in configdata:
      raise iutl.InstallError(
        "Undefined target: {} (please check \"{}\"".format(target, BUILD_CFGNAME))
    is_64bit = "is64bit" in configdata[target] and configdata[target]["is64bit"]
    if is_64bit:
      has_64bit = True
      continue
    BuildComponent(configdata[target], is_64bit, buildenv)
  if has_64bit:
    buildenv.update(iutl.GetVisualStudioEnvironment("x86_amd64"))
    for target in targetlist:
      if not target in configdata:
        raise iutl.InstallError(
          "Undefined target: {} (please check \"{}\"".format(target, BUILD_CFGNAME))
      is_64bit = "is64bit" in configdata[target] and configdata[target]["is64bit"]
      if is_64bit:
        BuildComponent(configdata[target], is_64bit, buildenv)


def CopyFile(filename, srcdir, dstdir, strip_first_dir=False):
  parts = filename.split("->")
  srcfilename = parts[0].strip()
  dstfilename = srcfilename if len(parts) == 1 else parts[1].strip()
  msg.Print(dstfilename)
  srcfile = os.path.normpath(os.path.join(srcdir, srcfilename))
  dstfile = os.path.normpath(dstfilename)
  if strip_first_dir:
    dstfile = os.sep.join(dstfile.split(os.path.sep)[1:])
  dstfile = os.path.join(dstdir, dstfile)
  utl.CopyAllFiles(srcfile, dstfile) \
    if srcfilename.endswith(("\\", "/")) else utl.CopyFile(srcfile, dstfile)


def CopyFiles(title, filelist, srcdir, dstdir, strip_first_dir=False):
  msg.Info("Copying {} files...".format(title))
  for filename in filelist:
    CopyFile(filename, srcdir, dstdir, strip_first_dir)


def PackSymbols():
  msg.Info("Packing symbols...")
  symbols_archive_name = "WindscribeSymbols_{}.zip".format(BUILD_APP_VERSION_STRINGS[0])
  zf = zipfile.ZipFile(symbols_archive_name, "w", zipfile.ZIP_DEFLATED)
  skiplen = len(BUILD_SYMBOL_FILES) + 1
  for filename in glob2.glob(BUILD_SYMBOL_FILES + os.sep + "**"):
    if os.path.isdir(filename):
      continue
    filenamepartial = filename[skiplen:]
    msg.Print(filenamepartial)
    zf.write(utl.MakeUnicodePath(filename), filenamepartial)
  zf.close()


def SignExecutables(filename_to_sign=None):
  msg.Info("Signing...")
  signtool = os.path.join(ROOT_DIR, "installer", "windows", "signing", "signtool.exe")
  certfile = os.path.join(ROOT_DIR, "installer", "windows", "signing", "code_signing.pfx")
  if filename_to_sign:
    iutl.RunCommand([signtool, "sign", "/t", "http://timestamp.digicert.com", "/f", certfile,
                   "/p", BUILD_CERT_PASSWORD, filename_to_sign])
  else:
    iutl.RunCommand([signtool, "sign", "/t", "http://timestamp.digicert.com", "/f", certfile,
                    "/p", BUILD_CERT_PASSWORD, os.path.join(BUILD_INSTALLER_FILES, "*.exe")])
    iutl.RunCommand([signtool, "sign", "/t", "http://timestamp.digicert.com", "/f", certfile,
                    "/p", BUILD_CERT_PASSWORD, os.path.join(BUILD_INSTALLER_FILES, "x32", "*.exe")])
    iutl.RunCommand([signtool, "sign", "/t", "http://timestamp.digicert.com", "/f", certfile,
                    "/p", BUILD_CERT_PASSWORD, os.path.join(BUILD_INSTALLER_FILES, "x64", "*.exe")])


def BuildInstallerWin32(configdata, qt_root, msvc_root, crt_root):
  # Copy Windows files.
  if "qt_files" in configdata:
    CopyFiles("Qt", configdata["qt_files"], qt_root, BUILD_INSTALLER_FILES, strip_first_dir=True)
  if "msvc_files" in configdata:
    CopyFiles("MSVC", configdata["msvc_files"], msvc_root, BUILD_INSTALLER_FILES)
  utl.CopyAllFiles(crt_root, BUILD_INSTALLER_FILES)
  if "additional_files" in configdata:
    additional_dir = os.path.join(ROOT_DIR, "installer", "windows", "additional_files")
    CopyFiles("additional", configdata["additional_files"], additional_dir, BUILD_INSTALLER_FILES)
  if "lib_files" in configdata:
    for k, v in configdata["lib_files"].iteritems():
      lib_root = iutl.GetDependencyBuildRoot(k)
      if not lib_root:
        raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
      CopyFiles(k, v, lib_root, BUILD_INSTALLER_FILES)
  # Pack symbols for crashdump analysis.
  PackSymbols()
  # Sign executable files with a certificate.
  SignExecutables()
  # Place everything in a 7z archive.
  msg.Info("Zipping...")
  installer_info = configdata[configdata["installer"]["win32"]]
  archive_filename = \
    os.path.normpath(os.path.join(ROOT_DIR, installer_info["subdir"], "../windscribe.7z"))
  print(archive_filename)
  ziptool = os.path.join(TOOLS_DIR, "bin", "7z.exe")
  iutl.RunCommand([ziptool, "a", archive_filename, os.path.join(BUILD_INSTALLER_FILES, "*"),
                   "-y", "-bso0", "-bsp2"])
  # Build and sign the installer.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  buildenv.update({ "CL" : "/MP" })
  BuildComponent(installer_info, False, buildenv)
  utl.RemoveFile(archive_filename)
  final_installer_name = os.path.normpath(os.path.join(os.getcwd(),
    "Windscribe_{}.exe".format(BUILD_APP_VERSION_STRINGS[0])))
  utl.RenameFile(os.path.normpath(os.path.join(BUILD_INSTALLER_FILES,
                  installer_info["target"])), final_installer_name)
  SignExecutables(final_installer_name)


def BuildAll():
  # Load config.
  configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, "{}".format(BUILD_CFGNAME)))
  if not configdata:
    raise iutl.InstallError("Failed to load config \"{}\".".format(BUILD_CFGNAME))
  current_os = utl.GetCurrentOS()
  if "targets" not in configdata or current_os not in configdata["targets"]:
    raise iutl.InstallError("No {} targets defined in \"{}\".".format(current_os, BUILD_CFGNAME))
  if "installer" not in configdata or \
      current_os not in configdata["installer"] or \
      configdata["installer"][current_os] not in configdata:
    raise iutl.InstallError(
      "Missing {} installer target in \"{}\".".format(current_os, BUILD_CFGNAME))
  # Extract app version.
  global BUILD_APP_VERSION_STRINGS
  BUILD_APP_VERSION_STRINGS = ExtractAppVersion()
  msg.Info("App version extracted: \"{}\"".format(BUILD_APP_VERSION_STRINGS[0]))
  # Get Qt directory.
  qt_root = iutl.GetDependencyBuildRoot("qt")
  if not qt_root:
    raise iutl.InstallError("Qt is not installed.")
  # Do some preliminary VS checks on Windows.
  if current_os == "win32":
    buildenv = os.environ.copy()
    buildenv.update(iutl.GetVisualStudioEnvironment())
    msvc_root = os.path.join(buildenv["VCTOOLSREDISTDIR"], "x86", "Microsoft.VC141.CRT")
    crt_root = "C:\\Program Files (x86)\\Windows Kits\\10\\Redist\\{}\\ucrt\\DLLS\\x86".format(
      buildenv["WINDOWSSDKVERSION"])
    if not os.path.exists(msvc_root):
      raise iutl.InstallError("MSVS installation not found.")
    if not os.path.exists(crt_root):
      raise iutl.InstallError("CRT files not found.")
  # Prepare output.
  artifact_dir = os.path.join(ROOT_DIR, "build-exe")
  utl.RemoveDirectory(artifact_dir)
  temp_dir = iutl.PrepareTempDirectory("installer")
  global BUILD_INSTALLER_FILES, BUILD_SYMBOL_FILES
  if current_os == "macos":
    BUILD_INSTALLER_FILES = os.path.join(ROOT_DIR, "installer", "mac", "binaries")
  else:
    BUILD_INSTALLER_FILES = os.path.join(temp_dir, "InstallerFiles")
  utl.CreateDirectory(BUILD_INSTALLER_FILES, True)
  if current_os == "win32": 
    BUILD_SYMBOL_FILES = os.path.join(temp_dir, "SymbolFiles")
    utl.CreateDirectory(BUILD_SYMBOL_FILES, True)
  # Build the components.
  GenerateProtobuf()
  old_cwd = os.getcwd()
  os.chdir(temp_dir)
  if configdata["targets"][current_os]:
    BuildComponents(configdata, configdata["targets"][current_os], qt_root)
  if current_os == "win32":
    BuildInstallerWin32(configdata, qt_root, msvc_root, crt_root)
  os.chdir(old_cwd)
  # Copy artifacts.
  msg.Print("Installing artifacts...")
  utl.CreateDirectory(artifact_dir, True)
  artifact_path = os.path.join(temp_dir, "InstallerFiles")
  for filename in glob2.glob(temp_dir + os.sep + "*"):
    if os.path.isdir(filename):
      continue
    filetitle = os.path.basename(filename)
    utl.CopyFile(filename, os.path.join(artifact_dir, filetitle))
    msg.HeadPrint("Ready: \"{}\"".format(filetitle))
  # Cleanup.
  msg.Print("Cleaning temporary directory...")
  utl.RemoveDirectory(temp_dir)


if __name__ == "__main__":
  start_time = time.time()
  current_os = utl.GetCurrentOS()
  if current_os not in BUILD_OS_LIST:
    msg.Print("{} is not needed on {}, skipping.".format(BUILD_TITLE, current_os))
    sys.exit(0)
  try:
    msg.Print("Building {}...".format(BUILD_TITLE))
    BuildAll()
    exitcode = 0
  except iutl.InstallError as e:
    msg.Error(e)
    exitcode = e.exitcode
  except IOError as e:
    msg.Error(e)
    exitcode = 1
  elapsed_time = time.time() - start_time
  if elapsed_time >= 60:
    msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time/60, elapsed_time%60))
  else:
    msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
  sys.exit(exitcode)
