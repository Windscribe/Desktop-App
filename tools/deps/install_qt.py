#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: installs Qt.
import multiprocessing
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, TOOLS_DIR)

CONFIG_NAME = os.path.join("vars", "qt.yml")

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import installutils as iutl

# Dependency-specific settings.
DEP_TITLE = "Qt"
DEP_URL = "https://download.qt.io/archive/qt/"
DEP_OS_LIST = ["win32", "macos", "linux"]
DEP_FILE_MASK = ["bin/**", "include/**", "lib/**", "libexec/**", "mkspecs/**", "phrasebooks/**", "plugins/**", "translations/**"]

QT_SKIP_MODULES = ["qtdoc", "qt3d", "qtactiveqt", "qtcanvas3d", "qtcharts", "qtconnectivity", "qtdatavis3d",
                   "qtdeclarative", "qtdoc", "qtgamepad", "qtgraphicaleffects", "qtgraphs", "qtlocation",
                   "qtnetworkauth", "qtpurchasing", "qtquickcontrols", "qtquickcontrols2",
                   "qtremoteobjects", "qtscript", "qtscxml", "qtserialbus", "qtserialport", "qtspeech",
                   "qtvirtualkeyboard", "qtwebchannel", "qtwebengine", "qtwebglplugin", "qtwebsockets",
                   "qtwebview", "qtlottie", "qtmqtt", "qtopcua", "qtquicktimeline", "qtquick3d", "qtcoap",
                   "qtpositioning", "qtsensors", "qtopengl", "qtquick3dphysics", "qtquickeffectmaker",
                   "qtlanguageserver", "qthttpserver", "qt5compat"]


def BuildDependencyMSVC(installpath, outpath):
    # Create an environment with VS vars.
    buildenv = os.environ.copy()
    buildenv.update(iutl.GetVisualStudioEnvironment())
    # Configure.
    configure_cmd = \
        ["configure.bat", "-static", "-static-runtime", "-no-openssl", "-schannel", "-c++std", "c++17", "-dbus-runtime", "-no-icu",
         "-no-glib", "-qt-doubleconversion", "-qt-pcre", "-qt-zlib", "-qt-freetype", "-qt-harfbuzz", "-qt-libpng", "-qt-libjpeg",
         "-qt-sqlite", "-qt-tiff", "-qt-webp", "-confirm-license", "-release", "-platform", "win32-msvc", "-nomake", "examples",
         "-nomake", "tests"]
    configure_cmd.extend(["-prefix", installpath])
    if QT_SKIP_MODULES:
        configure_cmd.extend(x for t in zip(["-skip"] * len(QT_SKIP_MODULES), QT_SKIP_MODULES) for x in t)
    configure_cmd.append("--")
    configure_cmd.append("-DQT_NO_MSVC_MIN_VERSION_CHECK=ON")
    iutl.RunCommand(configure_cmd, env=buildenv, shell=True)
    # Build and install.
    iutl.RunCommand(["cmake", "--build", ".", "--parallel"], env=buildenv, shell=True)
    iutl.RunCommand(["cmake", "--install", "."], env=buildenv, shell=True)


def BuildDependencyGNU(installpath, outpath):
    c_ismac = utl.GetCurrentOS() == "macos"
    # Create an environment.
    buildenv = os.environ.copy()

    # Configure.
    configure_cmd = \
        ["./configure", "-opensource", "-confirm-license", "-release", "-nomake", "examples"]

    configure_cmd.append("-qt-zlib")
    configure_cmd.append("-qt-pcre")
    configure_cmd.append("-no-icu")
    configure_cmd.append("-bundled-xcb-xinput")
    configure_cmd.extend(["-prefix", installpath])
    if not c_ismac:
        configure_cmd.append("-qt-libpng")
    if QT_SKIP_MODULES:
        configure_cmd.extend(x for t in zip(["-skip"] * len(QT_SKIP_MODULES), QT_SKIP_MODULES) for x in t)
    if c_ismac:
        configure_cmd.append("--")
        configure_cmd.append("-DCMAKE_OSX_ARCHITECTURES=\"x86_64;arm64\"")
        configure_cmd.append("-DQT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK=ON")
    iutl.RunCommand(configure_cmd, env=buildenv)

    # Build and install.
    iutl.RunCommand(["cmake", "--build", ".", "--parallel", str(multiprocessing.cpu_count())], env=buildenv)
    iutl.RunCommand(["cmake", "--install", "."], env=buildenv)


def InstallDependency():
    # Load environment.
    msg.HeadPrint("Loading: \"{}\"".format(CONFIG_NAME))
    configdata = utl.LoadConfig(os.path.join(TOOLS_DIR, CONFIG_NAME))
    if not configdata:
        raise iutl.InstallError("Failed to get config data.")
    iutl.SetupEnvironment(configdata)
    dep_name = DEP_TITLE.lower()
    dep_version_var = "VERSION_" + DEP_TITLE.upper().replace("-", "")
    dep_version_str = os.environ.get(dep_version_var, None)
    if not dep_version_str:
        raise iutl.InstallError("{} not defined.".format(dep_version_var))
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
    # Copy modified files.
    iutl.CopyCustomFiles(dep_name, os.path.join(temp_dir, archivetitle))
    # Build the dependency.
    dep_buildroot_var = "BUILDROOT_" + DEP_TITLE.upper()
    dep_buildroot_str = os.environ.get(dep_buildroot_var, os.path.join("build-libs", dep_name))
    outpath = os.path.normpath(os.path.join(os.path.dirname(TOOLS_DIR), dep_buildroot_str))
    # Clean the output folder to ensure no conflicts when we're updating to a newer Qt version.
    utl.RemoveDirectory(outpath)
    with utl.PushDir(os.path.join(temp_dir, extracteddir)):
        msg.HeadPrint("Building: \"{}\"".format(archivetitle))
        if utl.GetCurrentOS() == "win32":
            BuildDependencyMSVC(outpath, temp_dir)
        else:
            BuildDependencyGNU(outpath, temp_dir)
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
