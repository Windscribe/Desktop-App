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

NOTARIZE_FLAG = "--notarize"
CI_MODE_FLAG = "--ci-mode"

OPTION_CLEAN = "clean"

import base.messages as msg
import base.process as proc
import base.utils as utl
import deps.installutils as iutl

# Windscribe settings.
BUILD_TITLE = "Windscribe"
BUILD_CFGNAME = "build_all.yml"
BUILD_OS_LIST = ["win32", "macos", "linux"]
BUILD_CERT_PASSWORD = "fBafQVi0RC4Ts4zMUFOE" # TODO: keep elsewhere!
BUILD_DEVELOPER_MAC = "Developer ID Application: Windscribe Limited (GYZJYS7XUG)"

BUILD_APP_VERSION_STRINGS = ()
BUILD_QMAKE_EXE = ""
BUILD_MACDEPLOY = ""
BUILD_INSTALLER_FILES = ""
BUILD_SYMBOL_FILES = ""


def RemoveFiles(files):
  for f in files:
    try:
      if os.path.isfile(f):
        os.remove(f)
    except OSError as e:
      print("Error: %s : %s" % (f, e.strerror))

def RemoveEmptyDirs(dirs):
  for d in dirs:
    try:
      if len(os.listdir(d)) == 0 and os.path.isdir(d):
        os.rmdir(d)
      else:
        subdirs = []
        for subd in os.listdir(d):
          subdirs.append(os.path.join(d, subd))
        RemoveEmptyDirs(subdirs)
        os.rmdir(d)
    except OSError as e:
      print("Error: %s : %s" % (d, e.strerror))

def DeleteAllFiles(root_dir, dir_pattern):
  dirs_to_clean = glob2.glob(root_dir + os.sep + dir_pattern + "*")
  for dir in dirs_to_clean:
    RemoveFiles(glob2.glob(dir + "*/**/*" , recursive=True))
    RemoveFiles(glob2.glob(dir + "*/**/.*", recursive=True))
  RemoveEmptyDirs(dirs_to_clean)

def CleanAll():
  current_os = utl.GetCurrentOS()
  DeleteAllFiles(os.path.join(ROOT_DIR, "gui"), "build-gui-")
  DeleteAllFiles(os.path.join(ROOT_DIR, "backend"), "build-engine-")
  DeleteAllFiles(ROOT_DIR, "build-exe")
  DeleteAllFiles(ROOT_DIR, "temp")
  DeleteAllFiles(os.path.join(COMMON_DIR, "ipc"), "generated_proto")
  if current_os == "win32":
    DeleteAllFiles(os.path.join(ROOT_DIR, "backend", "windows", "windscribe_service"), "debug")
    DeleteAllFiles(os.path.join(ROOT_DIR, "backend", "windows", "windscribe_service"), "release")

def ExtractAppVersion():
  version_file = os.path.join(COMMON_DIR, "version", "windscribe_version.h")
  values = [0] * 4
  patterns = [re.compile("\\bWINDSCRIBE_MAJOR_VERSION\\s+(\\d+)"),
    re.compile("\\bWINDSCRIBE_MINOR_VERSION\\s+(\\d+)"),
    re.compile("\\bWINDSCRIBE_BUILD_VERSION\\s+(\\d+)"),
    re.compile("^#define\\s+WINDSCRIBE_IS_BETA")]
  with open(version_file, "r") as f:
    for line in f:
      for i in range(len(patterns)):
        matched = patterns[i].search(line)
        if matched:
          values[i] = int(matched.group(1)) if matched.lastindex > 0 else 1
          break
  version_string_1 = "{:d}_{:d}_build{:d}".format(values[0], values[1], values[2])
  version_string_2 = "{:d}.{:d}.{:d}".format(values[0], values[1], values[2])
  if values[3]:
    version_string_1 += "_beta"
  return (version_string_1, version_string_2)


def UpdateVersionInPlist(plistfilename):
  with open(plistfilename, "r") as file:
    filedata = file.read()
  # update Bundle Version
  filedata = re.sub("<key>CFBundleVersion</key>\n[^\n]+",
    "<key>CFBundleVersion</key>\n\t<string>{}</string>".format(BUILD_APP_VERSION_STRINGS[1]),
    filedata, flags = re.M)
  # update Bundle Version (short)
  filedata = re.sub("<key>CFBundleShortVersionString</key>\n[^\n]+",
    "<key>CFBundleShortVersionString</key>\n\t<string>{}</string>".format(BUILD_APP_VERSION_STRINGS[1]),
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

def FixRpathLinux(filename):
  parts = filename.split("->")
  srcfilename = parts[0].strip()
  rpath = "" if len(parts) == 1 else parts[1].strip()
  iutl.RunCommand(["patchelf", "--set-rpath", rpath, srcfilename])

def ApplyMacDeployFixes(appname, fixlist):
  # Special deploy fixes for Mac.
  # 1. copy_libs
  if "copy_libs" in fixlist:
    for k, v in fixlist["copy_libs"].iteritems():
      lib_root = iutl.GetDependencyBuildRoot(k)
      if not lib_root:
        raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
      CopyFiles(k, v, lib_root, appname)
  # 2. remove_files
  if "remove_files" in fixlist:
    msg.Info("Removing unnecessary files...")
    for k in fixlist["remove_files"]:
      utl.RemoveFile(os.path.join(appname, k))
  # 3. rpathfix
  if "rpathfix" in fixlist:
    with utl.PushDir():
      msg.Info("Fixing rpaths...")
      for f, m in fixlist["rpathfix"].iteritems():
        fs = os.path.split(f)
        os.chdir(os.path.join(appname,fs[0]))
        for k, v in m.iteritems():
          lib_root = iutl.GetDependencyBuildRoot(k)
          if not lib_root:
            raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
          for vv in v:
            parts = vv.split("->")
            srcv = parts[0].strip()
            dstv = srcv if len(parts) == 1 else parts[1].strip()
            change_lib_from = os.path.join(lib_root, srcv)
            change_lib_to = "@executable_path/{}".format(dstv)
            iutl.RunCommand(["install_name_tool", "-change", change_lib_from, change_lib_to, fs[1]])
  # 4. Code signing.
  if "codesign" in fixlist:
    # Signing the whole app.
    if "sign_app" in fixlist["codesign"] and fixlist["codesign"]["sign_app"]:
      msg.Info("Signing the app bundle...")
      iutl.RunCommand(["codesign", "--deep", appname, "--options", "runtime", "--timestamp",
                       "-s", BUILD_DEVELOPER_MAC])
      # This validation is optional.
      iutl.RunCommand(["codesign", "-v", appname])
    if "entitlements_binary" in fixlist["codesign"]:
      msg.Info("Signing a binary with entitlements...")
      entitlements_binary = os.path.join(appname, fixlist["codesign"]["entitlements_binary"])
      entitlements_file = os.path.join(ROOT_DIR, fixlist["codesign"]["entitlements_file"])
      iutl.RunCommand(["codesign", "--entitlements", entitlements_file, "-f",
                       "-s", BUILD_DEVELOPER_MAC, "--options", "runtime", "--timestamp",
                       entitlements_binary])


def BuildComponent(component, is_64bit, qt_root, buildenv=None, macdeployfixes=None, target_name_override=None):
  # Collect settings.
  current_os = utl.GetCurrentOS()
  c_iswin = current_os == "win32"
  c_ismac = current_os == "macos"
  c_bits = "64" if (is_64bit or c_ismac) else "32"
  c_project = component["project"]
  c_subdir = component["subdir"]
  c_target = component.get("target", None)
  if c_ismac and "macapp" in component:
    c_target = component["macapp"]
  # Strip "exe" if not on Windows.
  if not c_iswin and c_target.endswith(".exe"):
    c_target = c_target[:len(c_target) - 4]
  # Setup a directory.
  msg.Info("Building {} ({}-bit)...".format(component["name"], c_bits))
  with utl.PushDir() as current_wd:
    temp_wd = os.path.normpath(os.path.join(current_wd, c_subdir))
    utl.CreateDirectory(temp_wd)
    os.chdir(temp_wd)
    if c_project.endswith(".pro"):
      # Build Qt project.
      build_cmd = [BUILD_QMAKE_EXE, GetProjectFile(c_subdir, c_project), "CONFIG+=release silent"]
      if c_iswin:
        build_cmd.extend(["-spec", "win32-msvc"])
      iutl.RunCommand(build_cmd, env=buildenv, shell=c_iswin)
      iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv, shell=c_iswin)
      target_location = "release" if c_iswin else ""
      if c_ismac and "macapp" in component:
        deploy_cmd = [BUILD_MACDEPLOY, component["macapp"]]
        if "plugins" in component and not component["plugins"]:
          deploy_cmd.append("-no-plugins")
        iutl.RunCommand(deploy_cmd, env=buildenv)
        UpdateVersionInPlist(os.path.join(temp_wd,component["macapp"], "Contents", "Info.plist"))
    elif c_project.endswith(".vcxproj"):
      # Build MSVC project.
      conf = "Release_x64" if is_64bit else "Release"
      build_cmd = [
        "msbuild.exe", GetProjectFile(c_subdir, c_project),
        "/p:OutDir={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
        "/p:IntDir={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
        "/p:IntermediateOutputPath={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
        "/p:Configuration={}".format(conf), "-nologo", "-verbosity:m"
      ]
      iutl.RunCommand(build_cmd, env=buildenv, shell=True)
      target_location = "release-{}".format(c_bits)
    elif c_project.endswith(".xcodeproj"):
      # Build Xcode project.
      build_cmd = ["xcodebuild", "-scheme", component["scheme"], "-configuration", "Release", "-quiet"]
      if "xcflags" in component:
        build_cmd.extend(component["xcflags"])
      if buildenv and "ExternalCompilerOptions" in buildenv:
        build_cmd.append("OTHER_CFLAGS={}".format(buildenv["ExternalCompilerOptions"]))
      build_cmd.extend(["clean", "build"])
      os.chdir(os.path.join(ROOT_DIR, c_subdir))
      # use temp file to update version info so change isn't observed by version control
      temp_info_plist = ""
      if component["name"] == "Installer":
        utl.CopyFile("installer/Info.plist", "installer/temp_Info.plist")
        UpdateVersionInPlist("installer/temp_Info.plist")
        temp_info_plist = os.path.join(ROOT_DIR, c_subdir, "installer", "temp_Info.plist")
        build_cmd.extend(["INFOPLIST_FILE={}".format(temp_info_plist)])
      # build the project
      iutl.RunCommand(build_cmd, env=buildenv)
      # remove temp file -- no longer needed
      if temp_info_plist and os.path.exists(temp_info_plist):
        utl.RemoveFile(temp_info_plist)
      if c_target:
        outdir = proc.ExecuteAndGetOutput(["xcodebuild -project {} -showBuildSettings | " \
                                          "grep -m 1 \"BUILT_PRODUCTS_DIR\" | " \
                                          "grep -oEi \"\/.*\"".format(c_project)], shell=True)
        if c_target.endswith(".app"):
          utl.CopyMacBundle(os.path.join(outdir, c_target), os.path.join(temp_wd, c_target))
        else:
          utl.CopyFile(os.path.join(outdir, c_target), os.path.join(temp_wd, c_target))
      os.chdir(temp_wd)
      target_location = ""
    else:
      raise iutl.InstallError("Unknown project type: \"{}\"!".format(c_project))
    # Apply Mac deploy fixes to the app.
    if c_ismac and macdeployfixes and c_target.endswith(".app"):
      appfullname = os.path.join(temp_wd, target_location, c_target)
      ApplyMacDeployFixes(appfullname, macdeployfixes)
    # Copy output file.
    if c_target:
      srcfile = os.path.join(temp_wd, target_location, c_target)
      dstfile = BUILD_INSTALLER_FILES
      if "outdir" in component:
        dstfile = os.path.join(dstfile, component["outdir"])
      dstfile = os.path.join(dstfile, target_name_override if target_name_override else c_target)
      if c_target.endswith(".app"):
        utl.CopyMacBundle(srcfile, dstfile)
      else:
        utl.CopyFile(srcfile, dstfile)
    if BUILD_SYMBOL_FILES and "symbols" in component:
      srcfile = os.path.join(temp_wd, target_location, component["symbols"])
      if os.path.exists(srcfile):
        dstfile = BUILD_SYMBOL_FILES
        if "outdir" in component:
          dstfile = os.path.join(dstfile, component["outdir"])
        dstfile = os.path.join(dstfile, component["symbols"])
        utl.CopyFile(srcfile, dstfile)


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
      raise iutl.InstallError("Undefined target: {} (please check \"{}\"".format(target, BUILD_CFGNAME))
    is_64bit = "is64bit" in configdata[target] and configdata[target]["is64bit"]
    if is_64bit:
      has_64bit = True
      continue
    macdeployfixes = None
    if "macdeployfixes" in configdata and target in configdata["macdeployfixes"]:
      macdeployfixes = configdata["macdeployfixes"][target]
    BuildComponent(configdata[target], is_64bit, qt_root, buildenv, macdeployfixes)
  if has_64bit:
    buildenv.update(iutl.GetVisualStudioEnvironment("x86_amd64"))
    for target in targetlist:
      if not target in configdata:
        raise iutl.InstallError("Undefined target: {} (please check \"{}\"".format(target, BUILD_CFGNAME))
      is_64bit = "is64bit" in configdata[target] and configdata[target]["is64bit"]
      if is_64bit:
        macdeployfixes = None
        if "macdeployfixes" in configdata and target in configdata["macdeployfixes"]:
          macdeployfixes = configdata["macdeployfixes"][target]
        BuildComponent(configdata[target], is_64bit, qt_root, buildenv, macdeployfixes)


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


def SignExecutablesWin32(filename_to_sign=None):
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
  SignExecutablesWin32()
  # Place everything in a 7z archive.
  msg.Info("Zipping...")
  installer_info = configdata[configdata["installer"]["win32"]]
  archive_filename = os.path.normpath(os.path.join(ROOT_DIR, installer_info["subdir"], "../windscribe.7z"))
  print(archive_filename)
  ziptool = os.path.join(TOOLS_DIR, "bin", "7z.exe")
  iutl.RunCommand([ziptool, "a", archive_filename, os.path.join(BUILD_INSTALLER_FILES, "*"),
                   "-y", "-bso0", "-bsp2"])
  # Build and sign the installer.
  buildenv = os.environ.copy()
  buildenv.update({ "MAKEFLAGS" : "S" })
  buildenv.update(iutl.GetVisualStudioEnvironment())
  buildenv.update({ "CL" : "/MP" })
  BuildComponent(installer_info, False, qt_root, buildenv)
  utl.RemoveFile(archive_filename)
  final_installer_name = os.path.normpath(os.path.join(os.getcwd(),
    "Windscribe_{}.exe".format(BUILD_APP_VERSION_STRINGS[0])))
  utl.RenameFile(os.path.normpath(os.path.join(BUILD_INSTALLER_FILES,
                  installer_info["target"])), final_installer_name)
  SignExecutablesWin32(final_installer_name)


def BuildInstallerMac(configdata, qt_root):
  # Place everything in a 7z archive.
  msg.Info("Zipping...")
  installer_info = configdata[configdata["installer"]["macos"]]
  archive_filename = os.path.normpath(os.path.join(ROOT_DIR, installer_info["subdir"], "installer", "resources", "windscribe.7z"))
  utl.RemoveFile(archive_filename)
  iutl.RunCommand(["7z", "a", archive_filename,
                   os.path.join(BUILD_INSTALLER_FILES, "Windscribe.app"),
                   "-y", "-bso0", "-bsp2"])
  # Build and sign the installer.
  installer_app_override = "WindscribeInstaller.app"
  BuildComponent(installer_info, False, qt_root, target_name_override=installer_app_override)
  if NOTARIZE_FLAG in sys.argv:
    msg.Print("Notarizing...")
    notarize_script = os.path.join(TOOLS_DIR, "notarize.sh")
    iutl.RunCommand([notarize_script, CI_MODE_FLAG])
  # Drop DMG.
  msg.Print("Preparing dmg...")
  dmg_dir = BUILD_INSTALLER_FILES
  if "outdir" in installer_info:
    dmg_dir = os.path.join(dmg_dir, installer_info["outdir"])
  with utl.PushDir(dmg_dir):
    iutl.RunCommand(["dropdmg", "--config-name=Windscribe2", installer_app_override])
  final_installer_name = os.path.normpath(os.path.join(dmg_dir, "Windscribe_{}.dmg".format(BUILD_APP_VERSION_STRINGS[0])))
  utl.RenameFile(os.path.join(dmg_dir, "WindscribeInstaller.dmg"), final_installer_name)

def BuildInstallerLinux(configdata, qt_root):
  msg.Info("Copying lib_files_linux...")
  if "lib_files_linux" in configdata:
    for k, v in configdata["lib_files_linux"].iteritems():
      lib_root = iutl.GetDependencyBuildRoot(k)
      if not lib_root:
        raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
      CopyFiles(k, v, lib_root, BUILD_INSTALLER_FILES)

  msg.Info("Fixing rpaths...")
  if "files_fix_rpath_linux" in configdata:
    for k in configdata["files_fix_rpath_linux"]:
      dstfile = os.path.join(BUILD_INSTALLER_FILES, k)
      FixRpathLinux(dstfile)

  # Copy wstunnel
  msg.Info("Copying wstunnel...")
  wstunnel_dir = os.path.join(ROOT_DIR, "installer", "linux", "additional_files", "wstunnel")
  CopyFile("windscribewstunnel",wstunnel_dir, BUILD_INSTALLER_FILES)

  msg.Info("Creating Debian package...")
  src_package_path = os.path.join(ROOT_DIR, "installer", "linux", "debian_package")
  dest_package_path = os.path.join(BUILD_INSTALLER_FILES, "..", "windscribe_2.3-6_amd64")
  utl.CopyAllFiles(src_package_path, dest_package_path)
  utl.CopyAllFiles(BUILD_INSTALLER_FILES, os.path.join(dest_package_path, "usr", "local", "windscribe"))

  iutl.RunCommand(["fakeroot", "dpkg-deb", "--build", dest_package_path])


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
    raise iutl.InstallError("Missing {} installer target in \"{}\".".format(current_os, BUILD_CFGNAME))
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
    crt_root = "C:\\Program Files (x86)\\Windows Kits\\10\\Redist\\{}\\ucrt\\DLLS\\x86".format(buildenv["WINDOWSSDKVERSION"])
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
  with utl.PushDir(temp_dir):
    if configdata["targets"][current_os]:
      BuildComponents(configdata, configdata["targets"][current_os], qt_root)
    if current_os == "win32":
      BuildInstallerWin32(configdata, qt_root, msvc_root, crt_root)
    elif current_os == "macos":
      BuildInstallerMac(configdata, qt_root)
    elif current_os == "linux":
      BuildInstallerLinux(configdata, qt_root)
  # Copy artifacts.
  msg.Print("Installing artifacts...")
  utl.CreateDirectory(artifact_dir, True)

  if current_os == "macos":
    artifact_path = BUILD_INSTALLER_FILES
    installer_info = configdata[configdata["installer"]["macos"]]
    if "outdir" in installer_info:
      artifact_path = os.path.join(artifact_path, installer_info["outdir"])
  else:
    artifact_path = temp_dir

  #if current_os == "linux":
  #  utl.CopyAllFiles(BUILD_INSTALLER_FILES, artifact_dir)
  #else:
  for filename in glob2.glob(artifact_path + os.sep + "*"):
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
    if OPTION_CLEAN in sys.argv:
      msg.Print("Cleaning...")
      CleanAll()
    else:
      if current_os == "macos" and NOTARIZE_FLAG in sys.argv and not (CI_MODE_FLAG in sys.argv):
        msg.Print("Cannot notarize from build_all. Use manual notarization if necessary (may break offline notarizing check for user), but notarization should be done by the CI for permissions reasons.")
        sys.exit(0)
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
    msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time / 60, elapsed_time % 60))
  else:
    msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
  sys.exit(exitcode)
