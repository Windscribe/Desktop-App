#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: builds Windscribe.
import glob2
import os
import re
import time
import zipfile

import base.messages as msg
import base.process as proc
import base.utils as utl
import base.extract as extract
import base.secrethelper as secrethelper
import deps.installutils as iutl
from base.arghelper import *

import base.pathhelper as pathhelper
sys.path.insert(0, pathhelper.TOOLS_DIR)

# Windscribe settings.
BUILD_TITLE = "Windscribe"
BUILD_CFG_NAME = "build_all.yml"
BUILD_OS_LIST = ["win32", "macos", "linux"]

CURRENT_OS = ""

BUILD_QMAKE_EXE = ""
BUILD_MAC_DEPLOY = ""
BUILD_INSTALLER_FILES = ""
BUILD_SYMBOL_FILES = ""
MAC_DEV_ID_KEY_NAME = ""
MAC_DEV_TEAM_ID = ""
arghelper = ArgHelper(sys.argv)
extractor = extract.Extractor()


def remove_files(files):
    for f in files:
        try:
            if os.path.isfile(f):
                os.remove(f)
        except OSError as err:
            print(("Error: %s : %s" % (f, err.strerror)))


def remove_empty_dirs(dirs):
    for d in dirs:
        try:
            if len(os.listdir(d)) == 0 and os.path.isdir(d):
                os.rmdir(d)
            else:
                subdirs = []
                for subd in os.listdir(d):
                    subdirs.append(os.path.join(d, subd))
                remove_empty_dirs(subdirs)
                os.rmdir(d)
        except OSError as err:
            print(("Error: %s : %s" % (d, err.strerror)))


def delete_all_files(root_dir, dir_pattern):
    dirs_to_clean = glob2.glob(root_dir + os.sep + dir_pattern + "*")
    for directory in dirs_to_clean:
        remove_files(glob2.glob(directory + "*/**/*", recursive=True))
        remove_files(glob2.glob(directory + "*/**/.*", recursive=True))
    remove_empty_dirs(dirs_to_clean)


def clean_all_temp_and_build_dirs():
    delete_all_files(pathhelper.ROOT_DIR, "build-client-")
    delete_all_files(pathhelper.ROOT_DIR, "build-exe")
    delete_all_files(pathhelper.ROOT_DIR, "temp")
    delete_all_files(os.path.join(pathhelper.COMMON_DIR, "ipc"), "generated_proto")
    if CURRENT_OS == "win32":
        delete_all_files(os.path.join(pathhelper.ROOT_DIR, "backend", "windows", "windscribe_service"), "debug")
        delete_all_files(os.path.join(pathhelper.ROOT_DIR, "backend", "windows", "windscribe_service"), "release")


def generate_include_file_from_pub_key(key_filename_absolute, pubkey):
    with open(pubkey, "r") as infile:
        with open(key_filename_absolute, "w") as outfile:
            outfile.write("R\"(")
            for line in infile:
                outfile.write(line)
            outfile.write(")\"")


def update_version_in_plist(plistfilename):
    with open(plistfilename, "r") as f:
        file_data = f.read()
    # update Bundle Version
    file_data = re.sub("<key>CFBundleVersion</key>\n[^\n]+",
                       "<key>CFBundleVersion</key>\n\t<string>{}</string>"
                       .format(extractor.app_version()),
                       file_data, flags=re.M)
    # update Bundle Version (short)
    file_data = re.sub("<key>CFBundleShortVersionString</key>\n[^\n]+",
                       "<key>CFBundleShortVersionString</key>\n\t<string>{}</string>"
                       .format(extractor.app_version()),
                       file_data, flags=re.M)
    with open(plistfilename, "w") as f:
        f.write(file_data)


def update_version_in_debian_control(filename):
    with open(filename, "r") as f:
        filedata = f.read()
    # update Bundle Version
    filedata = re.sub("Version:[^\n]+",
                      "Version: {}".format(extractor.app_version()),
                      filedata, flags=re.M)
    with open(filename, "w") as f:
        f.write(filedata)


def update_team_id(filename):
    with open(filename, "r") as f:
        filedata = f.read()
    outdata = filedata.replace("$(DEVELOPMENT_TEAM)", MAC_DEV_TEAM_ID)
    with open(filename, "w") as f:
        f.write(outdata)


def restore_helper_info_plist(filename):
    with open(filename, "r") as f:
        filedata = f.read()
    outdata = filedata.replace(MAC_DEV_TEAM_ID, "$(DEVELOPMENT_TEAM)")
    with open(filename, "w") as f:
        f.write(outdata)


def get_project_file(subdir_name, project_name):
    return os.path.normpath(os.path.join(pathhelper.ROOT_DIR, subdir_name, project_name))


def get_project_folder(subdir_name):
    return os.path.normpath(os.path.join(pathhelper.ROOT_DIR, subdir_name))


def generate_protobuf():
    proto_root = iutl.GetDependencyBuildRoot("protobuf")
    if not proto_root:
        raise iutl.InstallError("Protobuf is not installed.")
    msg.Info("Generating Protobuf...")
    proto_gen = os.path.join(pathhelper.COMMON_DIR, "ipc", "proto", "generate_proto")
    if utl.GetCurrentOS() == "win32":
        proto_gen = proto_gen + ".bat"
        iutl.RunCommand([proto_gen, os.path.join(proto_root, "release", "bin")], shell=True)
    else:
        proto_gen = proto_gen + ".sh"
        iutl.RunCommand([proto_gen, os.path.join(proto_root, "bin")], shell=True)


def copy_file(filename, srcdir, dstdir, strip_first_dir=False):
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


def copy_files(title, filelist, srcdir, dstdir, strip_first_dir=False):
    msg.Info("Copying {} files...".format(title))
    for filename in filelist:
        copy_file(filename, srcdir, dstdir, strip_first_dir)


def fix_rpath_linux(filename):
    parts = filename.split("->")
    srcfilename = parts[0].strip()
    rpath = "" if len(parts) == 1 else parts[1].strip()
    iutl.RunCommand(["patchelf", "--set-rpath", rpath, srcfilename])


def apply_mac_deploy_fixes(appname, fixlist):
    # Special deploy fixes for Mac.
    # 1. copy_libs
    if "copy_libs" in fixlist:
        for k, v in fixlist["copy_libs"].items():
            lib_root = iutl.GetDependencyBuildRoot(k)
            if not lib_root:
                raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
            copy_files(k, v, lib_root, appname)
    # 2. remove_files
    if "remove_files" in fixlist:
        msg.Info("Removing unnecessary files...")
        for k in fixlist["remove_files"]:
            utl.RemoveFile(os.path.join(appname, k))
    # 3. rpathfix
    if "rpathfix" in fixlist:
        with utl.PushDir():
            msg.Info("Fixing rpaths...")
            for f, m in fixlist["rpathfix"].items():
                fs = os.path.split(f)
                os.chdir(os.path.join(appname, fs[0]))
                for k, v in m.items():
                    lib_root = iutl.GetDependencyBuildRoot(k)
                    if not lib_root:
                        raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
                    for vv in v:
                        parts = vv.split("->")
                        srcv = parts[0].strip()
                        dstv = srcv if len(parts) == 1 else parts[1].strip()
                        change_lib_from = os.path.join(lib_root, srcv)
                        change_lib_to = "@executable_path/{}".format(dstv)
                        msg.Info("Fixing rpath: {} to {} for {}".format(change_lib_from, change_lib_to, fs[1]))
                        # Ensure the lib actually exists (i.e. we have the correct name in build_all.yml)
                        if not os.path.exists(change_lib_from):
                            raise IOError("Cannot find file \"{}\"".format(change_lib_from))
                        iutl.RunCommand(["install_name_tool", "-change", change_lib_from, change_lib_to, fs[1]])
    # 4. Code signing.
    # The Mac app must be signed in order to install and operate properly.
    msg.Info("Signing the app bundle...")
    iutl.RunCommand(["codesign", "--deep", appname, "--options", "runtime", "--timestamp", "-s", MAC_DEV_ID_KEY_NAME])
    # This validation is optional.
    iutl.RunCommand(["codesign", "-v", appname])
    if "entitlements" in fixlist \
            and "entitlements_binary" in fixlist["entitlements"] \
            and "entitlements_file" in fixlist["entitlements"]:
        # Can only sign with entitlements if the embedded provisioning file exists.  The client will segfault on
        # launch otherwise with a "EXC_CRASH (Code Signature Invalid)" exception type.
        if os.path.exists(pathhelper.mac_provision_profile_filename_absolute()):
            msg.Info("Signing a binary with entitlements...")
            entitlements_binary = os.path.join(appname, fixlist["entitlements"]["entitlements_binary"])
            entitlements_file = os.path.join(pathhelper.ROOT_DIR, fixlist["entitlements"]["entitlements_file"])
            entitlements_file_temp = entitlements_file + "_temp"
            utl.CopyFile(entitlements_file, entitlements_file_temp)
            update_team_id(entitlements_file_temp)
            iutl.RunCommand(["codesign", "--entitlements", entitlements_file_temp, "-f",
                             "-s", MAC_DEV_ID_KEY_NAME, "--options", "runtime", "--timestamp",
                             entitlements_binary])
            utl.RemoveFile(entitlements_file_temp)
        else:
            msg.Warn("No embedded.provisionprofile found for this project.  IKEv2 will not function in this build.")


def build_component(component, is_64bit, qt_root, buildenv=None, macdeployfixes=None, target_name_override=None):
    # Collect settings.
    c_iswin = CURRENT_OS == "win32"
    c_ismac = CURRENT_OS == "macos"
    c_islinux = CURRENT_OS == utl.CURRENT_OS_LINUX;
    c_bits = "64" if (is_64bit or c_ismac or c_islinux) else "32"
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
            build_cmd = [BUILD_QMAKE_EXE, get_project_file(c_subdir, c_project), "CONFIG+=release silent"]
            if arghelper.sign_app():
                build_cmd.extend(["CONFIG+=use_signature_check"])
            if c_iswin:
                build_cmd.extend(["-spec", "win32-msvc"])
            if c_ismac:
                build_cmd.extend(["DEVELOPMENT_TEAM={}".format(MAC_DEV_TEAM_ID)])
            iutl.RunCommand(build_cmd, env=buildenv, shell=c_iswin)
            iutl.RunCommand(iutl.GetMakeBuildCommand(), env=buildenv, shell=c_iswin)
            target_location = "release" if c_iswin else ""
            if c_ismac and "macapp" in component:
                deploy_cmd = [BUILD_MAC_DEPLOY, component["macapp"]]
                if "plugins" in component and not component["plugins"]:
                    deploy_cmd.append("-no-plugins")
                iutl.RunCommand(deploy_cmd, env=buildenv)
                update_version_in_plist(os.path.join(temp_wd, component["macapp"], "Contents", "Info.plist"))
                if component["name"] == "Client":
                    # Could not find an automated way to do this like we could with the xcodebuild below.
                    update_team_id(os.path.join(temp_wd, component["macapp"], "Contents", "Info.plist"))
        elif c_project.endswith(".vcxproj"):
            # Build MSVC project.
            conf = "Release_x64" if is_64bit else "Release"
            build_cmd = [
                "msbuild.exe", get_project_file(c_subdir, c_project),
                "/p:OutDir={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
                "/p:IntDir={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
                "/p:IntermediateOutputPath={}release-{}{}".format(temp_wd + os.sep, c_bits, os.sep),
                "/p:Configuration={}".format(conf), "-nologo", "-verbosity:m"
            ]
            iutl.RunCommand(build_cmd, env=buildenv, shell=True)
            target_location = "release-{}".format(c_bits)
        elif c_project.endswith(".xcodeproj"):
            # Build Xcode project.
            team_w_id = "DEVELOPMENT_TEAM={}".format(MAC_DEV_TEAM_ID)
            build_cmd = ["xcodebuild", "-scheme", component["scheme"], "-configuration", "Release", "-quiet", team_w_id]
            if "xcflags" in component:
                build_cmd.extend(component["xcflags"])
            other_cflags = ""
            if arghelper.sign_app():
                other_cflags += "-DUSE_SIGNATURE_CHECK"
            if component["name"] == "Helper":
                # TODO: clean up all the warnings generated in the helper project.
                #       They impede our ability to spot legitimate warnings.
                other_cflags += " -Wno-incompatible-pointer-types-discards-qualifiers "\
                                "-Wno-strict-prototypes " \
                                "-Wno-enum-conversion " \
                                "-Wno-shorten-64-to-32 " \
                                "-Wno-incompatible-pointer-types"
                msg.Warn("Compiler warnings suppressed for this project.")
                # Update the team ID in the helper's plist.  xcodebuild won't do it for us as we are embedding the
                # plist via the Other Linker Flags section of the Xcode project.
                update_team_id(os.path.join(pathhelper.ROOT_DIR, c_subdir, "src", "helper-info.plist"))
            elif component["name"] == "Installer":
                # TODO: clean this warning at some point.
                other_cflags += " -Wno-deprecated-declarations -Wno-incomplete-umbrella"
                msg.Warn("Compiler warnings suppressed for this project.")
            if other_cflags:
                build_cmd.append("OTHER_CFLAGS=$(inherited) " + other_cflags)
            build_cmd.extend(["clean", "build"])
            os.chdir(os.path.join(pathhelper.ROOT_DIR, c_subdir))
            # use temp file to update version info so change isn't observed by version control
            temp_info_plist = ""
            if component["name"] == "Installer":
                utl.CopyFile("installer/Info.plist", "installer/temp_Info.plist")
                update_version_in_plist("installer/temp_Info.plist")
                temp_info_plist = os.path.join(pathhelper.ROOT_DIR, c_subdir, "installer", "temp_Info.plist")
                build_cmd.extend(["INFOPLIST_FILE={}".format(temp_info_plist)])
            build_exception = ""
            try:
                # build the project
                iutl.RunCommand(build_cmd, env=buildenv)
            except iutl.InstallError as e:
                build_exception = str(e)
            # remove temp file -- no longer needed
            if temp_info_plist and os.path.exists(temp_info_plist):
                utl.RemoveFile(temp_info_plist)
            if component["name"] == "Helper":
                # Undo what UpdateTeamID did above so version control doesn't see the change.
                restore_helper_info_plist(os.path.join(pathhelper.ROOT_DIR, c_subdir, "src", "helper-info.plist"))
            if build_exception:
                raise iutl.InstallError(build_exception)
            if c_target:
                outdir = proc.ExecuteAndGetOutput(["xcodebuild -project {} -showBuildSettings | " 
                                                   "grep -m 1 \"BUILT_PRODUCTS_DIR\" | " 
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
            apply_mac_deploy_fixes(appfullname, macdeployfixes)
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


def build_components(configdata, targetlist, qt_root):
    # Setup globals.
    global BUILD_QMAKE_EXE, BUILD_MAC_DEPLOY
    BUILD_QMAKE_EXE = os.path.join(qt_root, "bin", "qmake")
    if CURRENT_OS == "win32":
        BUILD_QMAKE_EXE += ".exe"
    BUILD_MAC_DEPLOY = os.path.join(qt_root, "bin", "macdeployqt")
    # Create an environment with compile-related vars.
    buildenv = os.environ.copy()
    if CURRENT_OS == "win32":
        buildenv.update({"MAKEFLAGS": "S"})
        buildenv.update(iutl.GetVisualStudioEnvironment())
        buildenv.update({"CL": "/MP"})
        if arghelper.sign_app():
            # Used by the windscribe_service Visual Studio project to enabled signature checking.
            buildenv.update({"ExternalCompilerOptions": "/DUSE_SIGNATURE_CHECK"})
    # Build all components needed.
    has_64bit = False
    for target in targetlist:
        if target not in configdata:
            raise iutl.InstallError("Undefined target: {} (please check \"{}\"".format(target, BUILD_CFG_NAME))
        is_64bit = "is64bit" in configdata[target] and configdata[target]["is64bit"]
        if is_64bit:
            has_64bit = True
            continue
        macdeployfixes = None
        if "macdeployfixes" in configdata and target in configdata["macdeployfixes"]:
            macdeployfixes = configdata["macdeployfixes"][target]
        build_component(configdata[target], is_64bit, qt_root, buildenv, macdeployfixes)
    if has_64bit:
        buildenv.update(iutl.GetVisualStudioEnvironment("x86_amd64"))
        for target in targetlist:
            if target not in configdata:
                raise iutl.InstallError("Undefined target: {} (please check \"{}\"".format(target, BUILD_CFG_NAME))
            is_64bit = "is64bit" in configdata[target] and configdata[target]["is64bit"]
            if is_64bit:
                macdeployfixes = None
                if "macdeployfixes" in configdata and target in configdata["macdeployfixes"]:
                    macdeployfixes = configdata["macdeployfixes"][target]
                build_component(configdata[target], is_64bit, qt_root, buildenv, macdeployfixes)


def build_auth_helper_win32(configdata, targetlist):
    # setup env
    buildenv = os.environ.copy()
    buildenv.update({"MAKEFLAGS": "S"})
    buildenv.update(iutl.GetVisualStudioEnvironment())
    buildenv.update({"CL": "/MP"})

    with utl.PushDir() as current_wd:
        msg.Print("Current working dir: " + current_wd)

        # ws_com, ws_com_server, ws_proxy_stub
        for target in targetlist:
            component = configdata[target]
            c_project = component["project"]
            c_subdir = component["subdir"]

            # creates structure similar to visual studio
            # >> important since authhelper components are dependent on ws_com.lib headers and links
            # >> (specified inside project file)
            # >> work/client-desktop/gui/authhelper/<projectname>/Release/<libs>
            build_cmd = [
                "msbuild.exe", get_project_file(c_subdir, c_project),
                "/p:OutDir=..\Release{}".format(os.sep),
                "/p:Configuration={}".format("Release"), "-nologo", "-verbosity:m"
            ]
            iutl.RunCommand(build_cmd, env=buildenv, shell=True)

            # move necessary outputs to InstallerFiles (for deployment)
            release_target = os.path.join(get_project_folder(c_subdir), "..", "Release", component["target"])
            src_target_name = os.path.normpath(release_target)
            dest_target_name = os.path.join(BUILD_INSTALLER_FILES, component["target"])
            msg.Verbose("Moving " + src_target_name + " -> " + dest_target_name)
            utl.CopyFile(src_target_name, dest_target_name)


def pack_symbols():
    msg.Info("Packing symbols...")
    symbols_archive_name = "WindscribeSymbols_{}.zip".format(extractor.app_version(True))
    zf = zipfile.ZipFile(symbols_archive_name, "w", zipfile.ZIP_DEFLATED)
    skiplen = len(BUILD_SYMBOL_FILES) + 1
    for filename in glob2.glob(BUILD_SYMBOL_FILES + os.sep + "**"):
        if os.path.isdir(filename):
            continue
        filenamepartial = filename[skiplen:]
        msg.Print(filenamepartial)
        zf.write(utl.MakeUnicodePath(filename), filenamepartial)
    zf.close()


def sign_executables_win32(configdata, cert_password, filename_to_sign=None):
    if "windows_signing_cert" in configdata and \
            "path_signtool" in configdata["windows_signing_cert"] and \
            "path_cert" in configdata["windows_signing_cert"] and \
            "timestamp" in configdata["windows_signing_cert"] and \
            cert_password:
        signtool = os.path.join(pathhelper.ROOT_DIR, configdata["windows_signing_cert"]["path_signtool"])
        certfile = os.path.join(pathhelper.ROOT_DIR, configdata["windows_signing_cert"]["path_cert"])
        timestamp = configdata["windows_signing_cert"]["timestamp"]

        msg.Info("Signing...")
        if filename_to_sign:
            iutl.RunCommand([signtool, "sign", "/t", timestamp, "/f", certfile,
                             "/p", cert_password, filename_to_sign])
        else:
            iutl.RunCommand([signtool, "sign", "/t", timestamp, "/f", certfile,
                             "/p", cert_password, os.path.join(BUILD_INSTALLER_FILES, "*.exe")])
            iutl.RunCommand([signtool, "sign", "/t", timestamp, "/f", certfile,
                             "/p", cert_password, os.path.join(BUILD_INSTALLER_FILES, "x32", "*.exe")])
            iutl.RunCommand([signtool, "sign", "/t", timestamp, "/f", certfile,
                             "/p", cert_password, os.path.join(BUILD_INSTALLER_FILES, "x64", "*.exe")])
    else:
        msg.Info("Skip signing. The signing data is not set in YML.")


def build_installer_win32(configdata, qt_root, msvc_root, crt_root, win_cert_password):
    # Copy Windows files.
    if "qt_files" in configdata:
        copy_files("Qt", configdata["qt_files"], qt_root, BUILD_INSTALLER_FILES, strip_first_dir=True)
    if "msvc_files" in configdata:
        copy_files("MSVC", configdata["msvc_files"], msvc_root, BUILD_INSTALLER_FILES)
    utl.CopyAllFiles(crt_root, BUILD_INSTALLER_FILES)
    if "additional_files" in configdata:
        additional_dir = os.path.join(pathhelper.ROOT_DIR, "installer", "windows", "additional_files")
        copy_files("additional", configdata["additional_files"], additional_dir, BUILD_INSTALLER_FILES)
    if "lib_files" in configdata:
        for k, v in configdata["lib_files"].items():
            lib_root = iutl.GetDependencyBuildRoot(k)
            if not lib_root:
                raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
            copy_files(k, v, lib_root, BUILD_INSTALLER_FILES)
    if "license_files" in configdata:
        license_dir = os.path.join(pathhelper.COMMON_DIR, "licenses")
        copy_files("license", configdata["license_files"], license_dir, BUILD_INSTALLER_FILES)
    # Pack symbols for crashdump analysis.
    pack_symbols()

    if arghelper.sign_app():
        # Sign executable files with a certificate.
        sign_executables_win32(configdata, win_cert_password)
        # Sign AuthHelper DLLs
        if arghelper.build_com():
            auth_helper_com_target = os.path.join(BUILD_INSTALLER_FILES, configdata["authhelper_com"]["target"])
            sign_executables_win32(configdata, win_cert_password, auth_helper_com_target)
            com_proxy_stub = os.path.join(BUILD_INSTALLER_FILES, configdata["authhelper_com_proxy_stub"]["target"])
            sign_executables_win32(configdata, win_cert_password, com_proxy_stub)
    # Place everything in a 7z archive.
    msg.Info("Zipping...")
    installer_info = configdata[configdata["installer"]["win32"]]
    archive_filename = os.path.normpath(os.path.join(pathhelper.ROOT_DIR, installer_info["subdir"], "../windscribe.7z"))
    print(archive_filename)
    ziptool = os.path.join(pathhelper.TOOLS_DIR, "bin", "7z.exe")
    iutl.RunCommand([ziptool, "a", archive_filename, os.path.join(BUILD_INSTALLER_FILES, "*"),
                     "-y", "-bso0", "-bsp2"])
    # Build and sign the installer.
    buildenv = os.environ.copy()
    buildenv.update({"MAKEFLAGS": "S"})
    buildenv.update(iutl.GetVisualStudioEnvironment())
    buildenv.update({"CL": "/MP"})
    build_component(installer_info, False, qt_root, buildenv)
    if arghelper.post_clean():
        utl.RemoveFile(archive_filename)
    final_installer_name = os.path.normpath(os.path.join(os.getcwd(),
                                                         "Windscribe_{}.exe".format(extractor.app_version(True))))
    msg.Info("App version extracted22: \"{}\"".format(extractor.app_version(True)))
    msg.Info("App version extracted33: \"{}\"".format(final_installer_name))

    utl.RenameFile(os.path.normpath(os.path.join(BUILD_INSTALLER_FILES,
                                                 installer_info["target"])), final_installer_name)
    if arghelper.sign_app():
        sign_executables_win32(configdata, win_cert_password, final_installer_name)


def build_installer_mac(configdata, qt_root):
    # Place everything in a 7z archive.
    msg.Info("Zipping...")
    installer_info = configdata[configdata["installer"]["macos"]]
    arc_path = os.path.join(pathhelper.ROOT_DIR, installer_info["subdir"], "installer", "resources", "windscribe.7z")
    archive_filename = os.path.normpath(arc_path)
    if arghelper.post_clean():
        utl.RemoveFile(archive_filename)
    iutl.RunCommand(["7z", "a", archive_filename,
                     os.path.join(BUILD_INSTALLER_FILES, "Windscribe.app"),
                     "-y", "-bso0", "-bsp2"])
    # Build and sign the installer.
    installer_app_override = "WindscribeInstaller.app"
    build_component(installer_info, False, qt_root, target_name_override=installer_app_override)
    if arghelper.notarize():
        msg.Print("Notarizing...")
        iutl.RunCommand([pathhelper.notarize_script_filename_absolute(), arghelper.OPTION_CI_MODE])
    # Drop DMG.
    msg.Print("Preparing dmg...")
    dmg_dir = BUILD_INSTALLER_FILES
    if "outdir" in installer_info:
        dmg_dir = os.path.join(dmg_dir, installer_info["outdir"])
    with utl.PushDir(dmg_dir):
        iutl.RunCommand(["python", "-m", "dmgbuild", "-s",
                         pathhelper.ROOT_DIR + "/installer/mac/dmgbuild/dmgbuild_settings.py",
                         "WindscribeInstaller",
                         "WindscribeInstaller.dmg", "-D", "app=" + installer_app_override, "-D",
                         "background=" + pathhelper.ROOT_DIR + "/installer/mac/dmgbuild/osx_install_background.tiff"])
        final_installer_name = os.path.normpath(os.path.join(dmg_dir, "Windscribe_{}.dmg"
                                                         .format(extractor.app_version(True))))
    utl.RenameFile(os.path.join(dmg_dir, "WindscribeInstaller.dmg"), final_installer_name)


def code_sign_linux(binary_name, binary_dir, signature_output_dir):
    binary = binary_dir + "/" + binary_name
    private_key = pathhelper.ROOT_DIR + "/common/keys/linux/key.pem"
    signature = signature_output_dir + "/" + binary_name + ".sig"
    msg.Info("Signing " + binary + " with " + private_key + " -> " + signature)
    cmd = ["openssl", "dgst", "-sign", private_key, "-keyform", "PEM", "-sha256", "-out", signature, "-binary", binary]
    iutl.RunCommand(cmd)


def build_installer_linux(configdata, qt_root):
    # Creates the following:
    # * windscribe_2.x.y_amd64.deb
    # * windscribe_2.x.y_x86_64.rpm
    msg.Info("Copying lib_files_linux...")
    if "lib_files_linux" in configdata:
        for k, v in configdata["lib_files_linux"].items():
            lib_root = iutl.GetDependencyBuildRoot(k)
            if not lib_root:
                raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
            copy_files(k, v, lib_root, BUILD_INSTALLER_FILES)

    msg.Info("Fixing rpaths...")
    if "files_fix_rpath_linux" in configdata:
        for k in configdata["files_fix_rpath_linux"]:
            dstfile = os.path.join(BUILD_INSTALLER_FILES, k)
            fix_rpath_linux(dstfile)

    # Copy wstunnel into InstallerFiles
    msg.Info("Copying wstunnel...")
    wstunnel_dir = os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "additional_files", "wstunnel")
    copy_file("windscribewstunnel", wstunnel_dir, BUILD_INSTALLER_FILES)

    # sign supplementary binaries and move the signatures into InstallerFiles/signatures
    if arghelper.sign_app():
        signatures_dir = os.path.join(BUILD_INSTALLER_FILES, "signatures")
        msg.Print("Creating signatures path: " + signatures_dir)
        utl.CreateDirectory(signatures_dir, True)
        if "files_codesign_linux" in configdata:
            for binary_name in configdata["files_codesign_linux"]:
                code_sign_linux(binary_name, BUILD_INSTALLER_FILES, signatures_dir)

    if "license_files" in configdata:
        license_dir = os.path.join(pathhelper.COMMON_DIR, "licenses")
        copy_files("license", configdata["license_files"], license_dir, BUILD_INSTALLER_FILES)

    msg.Info("Creating Debian package...")
    src_package_path = os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "debian_package")
    dest_package_name = "windscribe_{}_amd64".format(extractor.app_version(True))
    dest_package_path = os.path.join(BUILD_INSTALLER_FILES, "..", dest_package_name)

    utl.CopyAllFiles(src_package_path, dest_package_path)
    utl.CopyAllFiles(BUILD_INSTALLER_FILES, os.path.join(dest_package_path, "usr", "local", "windscribe"))

    update_version_in_debian_control(os.path.join(dest_package_path, "DEBIAN", "control"))

    # create .deb with dest_package
    # Force use of 'xz' compression.  dpkg on Ubuntu 21.10 defaulting to zstd compression,
    # which fpm currently cannot handle.
    iutl.RunCommand(["fakeroot", "dpkg-deb", "-Zxz", "--build", dest_package_path])
    
    # create RPM from deb
    # msg.Info("Creating RPM package...")
    rpm_package_name = "windscribe_{}_x86_64.rpm".format(extractor.app_version(True))
    postinst_rpm_script = os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "additional_files", "postinst_rpm")
    iutl.RunCommand(["fpm", "--after-install", postinst_rpm_script,
                     "-s", "deb",
                     "-p", rpm_package_name,
                     "-t", "rpm",
                     dest_package_path + ".deb"])


def build_all(win_cert_password):
    # Load config.
    configdata = utl.LoadConfig(os.path.join(pathhelper.TOOLS_DIR, "{}".format(BUILD_CFG_NAME)))
    if not configdata:
        raise iutl.InstallError("Failed to load config \"{}\".".format(BUILD_CFG_NAME))
    if "targets" not in configdata or CURRENT_OS not in configdata["targets"]:
        raise iutl.InstallError("No {} targets defined in \"{}\".".format(CURRENT_OS, BUILD_CFG_NAME))
    if "installer" not in configdata or \
            CURRENT_OS not in configdata["installer"] or \
            configdata["installer"][CURRENT_OS] not in configdata:
        raise iutl.InstallError("Missing {} installer target in \"{}\".".format(CURRENT_OS, BUILD_CFG_NAME))

    msg.Info("App version extracted: \"{}\"".format(extractor.app_version(True)))

    # Get Qt directory.
    qt_root = iutl.GetDependencyBuildRoot("qt")
    if not qt_root:
        raise iutl.InstallError("Qt is not installed.")

    # Do some preliminary VS checks on Windows.
    if CURRENT_OS == "win32":
        buildenv = os.environ.copy()
        buildenv.update(iutl.GetVisualStudioEnvironment())
        msvc_root = os.path.join(buildenv["VCTOOLSREDISTDIR"], "x86", "Microsoft.VC141.CRT")
        crt_root = "C:\\Program Files (x86)\\Windows Kits\\10\\Redist\\{}\\ucrt\\DLLS\\x86"\
            .format(buildenv["WINDOWSSDKVERSION"])
        if not os.path.exists(msvc_root):
            raise iutl.InstallError("MSVS installation not found.")
        if not os.path.exists(crt_root):
            raise iutl.InstallError("CRT files not found.")

    # Prepare output.
    artifact_dir = os.path.join(pathhelper.ROOT_DIR, "build-exe")
    if arghelper.post_clean():
        utl.RemoveDirectory(artifact_dir)
    temp_dir = iutl.PrepareTempDirectory("installer")
    global BUILD_INSTALLER_FILES, BUILD_SYMBOL_FILES
    if CURRENT_OS == "macos":
        BUILD_INSTALLER_FILES = os.path.join(pathhelper.ROOT_DIR, "installer", "mac", "binaries")
    else:
        BUILD_INSTALLER_FILES = os.path.join(temp_dir, "InstallerFiles")
    utl.CreateDirectory(BUILD_INSTALLER_FILES, arghelper.post_clean())
    if CURRENT_OS == "win32":
        BUILD_SYMBOL_FILES = os.path.join(temp_dir, "SymbolFiles")
        utl.CreateDirectory(BUILD_SYMBOL_FILES, arghelper.post_clean())

    # Build the components.
    generate_protobuf()
    with utl.PushDir(temp_dir):
        if arghelper.build_app():
            if configdata["targets"][CURRENT_OS]:
                build_components(configdata, configdata["targets"][CURRENT_OS], qt_root)
        if CURRENT_OS == "win32":
            if arghelper.build_com():
                build_auth_helper_win32(configdata, configdata["targets"]["authhelper_win"])
            if arghelper.build_installer():
                build_installer_win32(configdata, qt_root, msvc_root, crt_root, win_cert_password)
        elif CURRENT_OS == "macos":
            if arghelper.build_installer():
                build_installer_mac(configdata, qt_root)
        elif CURRENT_OS == "linux":
            build_installer_linux(configdata, qt_root)

    # Copy artifacts.
    msg.Print("Installing artifacts...")
    utl.CreateDirectory(artifact_dir, arghelper.post_clean())
    msg.Print("Installing artifacts...")
    if CURRENT_OS == "macos":
        artifact_path = BUILD_INSTALLER_FILES
        installer_info = configdata[configdata["installer"]["macos"]]
        if "outdir" in installer_info:
            artifact_path = os.path.join(artifact_path, installer_info["outdir"])
    else:
        artifact_path = temp_dir
    for filename in glob2.glob(artifact_path + os.sep + "*"):
        if os.path.isdir(filename):
            continue
        filetitle = os.path.basename(filename)
        utl.CopyFile(filename, os.path.join(artifact_dir, filetitle))
        msg.HeadPrint("Ready: \"{}\"".format(filetitle))

    # Cleanup.
    if arghelper.post_clean():
        msg.Print("Cleaning temporary directory...")
        utl.RemoveDirectory(temp_dir)


def download_secrets():
    msg.Print("Checking for valid 1password username")
    if not arghelper.valid_one_password_username():
        user = arghelper.OPTION_ONE_PASSWORD_USER
        raise IOError("Specify 1password username with " + user + " <username> or do not sign")
    msg.Print("Logging in to 1password to download files necessary for signing")
    secrethelper.login_and_download_files_from_1password(arghelper)


def pre_checks_and_build_all():
    # USER ERROR - Cannot notarize without signing
    if arghelper.notarize() and not arghelper.sign_app():
        raise IOError("Cannot notarize without signing. Please enable signing or disable notarization")

    # USER INFO - can't notarize on non-Mac
    if CURRENT_OS != utl.CURRENT_OS_MAC and arghelper.notarize():
        msg.Info("Notarization does not exist on non-Mac platforms, notarization will be skipped")

    # USER ERROR: notarization must be run with sudo perms when locally building and cannot be called from build_all
    if arghelper.notarize() and not arghelper.ci_mode():
        raise IOError("Cannot notarize from build_all. Use manual notarization if necessary "
                      "(may break offline notarizing check for user), "
                      "but notarization should be done by the CI for permissions reasons.")

    # TODO: add check for code-sign cert (in keychain) on mac (check during --no-sign as well)
    # get required Apple developer info
    if CURRENT_OS == "macos":
        global MAC_DEV_ID_KEY_NAME
        global MAC_DEV_TEAM_ID
        MAC_DEV_ID_KEY_NAME, MAC_DEV_TEAM_ID = extractor.mac_signing_params()
        msg.Info("using signing identity - " + MAC_DEV_ID_KEY_NAME)

    win_cert_password = ""
    if arghelper.sign_app():
        # download (local build) or access secret files (ci build)
        if not arghelper.ci_mode():
            if not arghelper.use_local_secrets():
                download_secrets()
                
        # on linux we need keypair to sign -- check that they exist in the correct location
        if CURRENT_OS == utl.CURRENT_OS_LINUX:
            keypath = pathhelper.linux_key_directory()
            pubkey = pathhelper.linux_public_key_filename_absolute()
            privkey = pathhelper.linux_private_key_filename_absolute()
            if not os.path.exists(pubkey) or not os.path.exists(privkey):
                raise IOError("Code signing (--sign) is enabled but key.pub and/or key.pem were not found in '{keypath}'."
                              .format(keypath=keypath))
            generate_include_file_from_pub_key(pathhelper.linux_include_key_filename_absolute(), pubkey)

        # early check for cert password in notarize.yml on windows
        if CURRENT_OS == utl.CURRENT_OS_WIN:
            notarize_yml_path = os.path.join(pathhelper.TOOLS_DIR, "{}".format(pathhelper.NOTARIZE_YML))
            notarize_yml_config = utl.LoadConfig(notarize_yml_path)
            if not notarize_yml_config:
                raise IOError("Cannot sign without notarize.yml on Windows")
            win_cert_password = notarize_yml_config["windows-signing-cert-password"]
            if not win_cert_password:
                raise IOError("Cannot sign with invalid Windows cert password")

        # early check for provision profile on mac
        if CURRENT_OS == utl.CURRENT_OS_MAC:
            if not os.path.exists(pathhelper.mac_provision_profile_filename_absolute()):
                raise IOError("Cannot sign without provisioning profile")

	# early check for file hardcodedsecrets.ini
        if not os.path.exists(pathhelper.hardcoded_secrets_filename_absolute()):
    	    raise IOError("Cannot build without hardcodedsecrets.ini")

    # should have everything we need to build with the desired settings
    msg.Print("Building {}...".format(BUILD_TITLE))
    build_all(win_cert_password)


def print_banner(title, width):
    msg.Print("*" * width)
    msg.Print(title)
    msg.Print("*" * width)


def print_help_output():
    print_banner("Windscribe build_all help menu", 60)
    msg.Print("Windscribe's build_all script is used to build and sign the desktop app for Windows, Mac and Linux.")
    msg.Print("By default it will attempt to build without signing.")

    # print all the options
    for name, description in arghelper.options:
        msg.Print(name + ": " + description)
    msg.Print("")


# main script logic, secret cleanup, and timing display
if __name__ == "__main__":
    start_time = time.time()
    CURRENT_OS = utl.GetCurrentOS()

    delete_secrets = False
    try:
        if CURRENT_OS not in BUILD_OS_LIST:
            raise IOError("Building {} is not supported on {}.".format(BUILD_TITLE, CURRENT_OS))

        # check for invalid modes
        err = arghelper.invalid_mode()
        if err:
            raise IOError(err)

        # main decision logic
        if arghelper.help():
            print_help_output()
        elif arghelper.download_secrets():
            download_secrets()
        elif arghelper.build_mode():
            # don't delete secrets in local-secret mode or when doing a developer (non-signed) build.
            if not arghelper.use_local_secrets() and arghelper.sign_app():
                delete_secrets = True
            pre_checks_and_build_all()
        else:
            if arghelper.clean_only():
                msg.Print("Cleaning...")
                clean_all_temp_and_build_dirs()
            if arghelper.delete_secrets_only():
                delete_secrets = True
        exitcode = 0
    except iutl.InstallError as e:
        msg.Error(e)
        exitcode = e.exitcode
    except IOError as e:
        msg.Error(e)
        exitcode = 1

    # delete secrets if necessary
    if delete_secrets:
        msg.Print("Deleting secrets...")
        secrethelper.cleanup_secrets()

    # timing output
    if exitcode == 0:
        elapsed_time = time.time() - start_time
        if elapsed_time >= 60:
            msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time / 60, elapsed_time % 60))
        else:
            msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
    sys.exit(exitcode)
