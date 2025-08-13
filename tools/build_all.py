#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2025, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: builds Windscribe.
import glob
import os
import pathlib
import platform
import re
import subprocess
import sys
import time
import zipfile
import multiprocessing

from pathlib import Path

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.process as proc
import base.utils as utl
import base.extract as extract
import deps.installutils as iutl
from base.arghelper import ArgHelper

# Windscribe settings.
BUILD_TITLE = "Windscribe"
BUILD_CFG_NAME = "build_all.yml"
BUILD_OS_LIST = ["win32", "macos", "linux"]

CURRENT_OS = ""
CURRENT_VCPKG_TRIPLET = ""

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
            print("Error: %s : %s" % (f, err.strerror))


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
            print("Error: %s : %s" % (d, err.strerror))


def delete_all_files(root_dir, dir_pattern):
    dirs_to_clean = glob.glob(root_dir + os.sep + dir_pattern + "*")
    for directory in dirs_to_clean:
        remove_files(glob.glob(directory + "*/**/*", recursive=True))
        remove_files(glob.glob(directory + "*/**/.*", recursive=True))
    remove_empty_dirs(dirs_to_clean)


def clean_all_temp_and_build_dirs():
    if (CURRENT_OS == "win32"):
        delete_all_files(pathhelper.ROOT_DIR, "build-" + CURRENT_OS + ("-arm64" if arghelper.target_arm64_arch() else ""))
    else:
        delete_all_files(pathhelper.ROOT_DIR, "build")
    delete_all_files(pathhelper.ROOT_DIR, "build-exe")
    delete_all_files(pathhelper.ROOT_DIR, "temp")


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


def update_version_in_config(filename):
    with open(filename, "r") as f:
        filedata = f.read()
    # update Bundle Version
    filedata = re.sub("Version:[^\n]+",
                      "Version: {}".format(extractor.app_version()),
                      filedata, flags=re.M)
    with open(filename, "w") as f:
        f.write(filedata)


def update_arch_in_config(filename):
    with open(filename, "r") as f:
        filedata = f.read()
    # update Bundle Version
    if platform.machine() == "x86_64":
        filedata = re.sub("Architecture:[^\n]+", "Architecture: amd64", filedata, flags=re.M)
    elif platform.machine() == "aarch64":
        filedata = re.sub("Architecture:[^\n]+", "Architecture: arm64", filedata, flags=re.M)
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


def apply_mac_deploy_fixes(configdata, target, appname, fullpath):
    # Special deploy fixes for Mac.
    # 1. copy_libs
    copy_libs(configdata, "macos", target, fullpath)
    # 2. remove_files
    if "remove" in configdata["deploy_files"]["macos"][target]:
        msg.Info("Removing unnecessary files...")
        for k in configdata["deploy_files"]["macos"][target]["remove"]:
            utl.RemoveFile(os.path.join(appname, k))
    # 3. Code signing.
    # The Mac app must be signed in order to install and operate properly.
    msg.Info("Signing the app bundle...")
    iutl.RunCommand(["codesign", "--deep", fullpath, "--options", "runtime", "--timestamp", "-s", MAC_DEV_ID_KEY_NAME, "-f"])


def sign_mac_entitlements(configdata, target, appname, fullpath):
    # This validation is optional.
    iutl.RunCommand(["codesign", "-v", fullpath])
    if "entitlements" in configdata["deploy_files"]["macos"][target] \
            and "entitlements_binary" in configdata["deploy_files"]["macos"][target]["entitlements"] \
            and "entitlements_file" in configdata["deploy_files"]["macos"][target]["entitlements"]:
        # Can only sign with entitlements if the embedded provisioning file exists.  The client will segfault on
        # launch otherwise with a "EXC_CRASH (Code Signature Invalid)" exception type.
        provision_profile = os.path.join(pathhelper.ROOT_DIR, configdata["deploy_files"]["macos"][target]["entitlements"]["provision_profile"])
        if os.path.exists(provision_profile):
            msg.Info("Signing a binary with entitlements...")
            entitlements_binary = os.path.join(fullpath, configdata["deploy_files"]["macos"][target]["entitlements"]["entitlements_binary"])
            entitlements_file = os.path.join(pathhelper.ROOT_DIR, configdata["deploy_files"]["macos"][target]["entitlements"]["entitlements_file"])
            entitlements_file_temp = entitlements_file + "_temp"
            utl.CopyFile(entitlements_file, entitlements_file_temp)
            update_team_id(entitlements_file_temp)
            iutl.RunCommand(["codesign", "--entitlements", entitlements_file_temp, "-f",
                             "-s", MAC_DEV_ID_KEY_NAME, "--options", "runtime", "--timestamp",
                             entitlements_binary])
            utl.RemoveFile(entitlements_file_temp)
        else:
            msg.Warn("No provisioning profile ({}) found for this project.".format(provision_profile))


def build_component(component, buildenv=None):
    msg.Info("Building {}...".format(component["name"]))
    with utl.PushDir() as current_wd:
        temp_wd = os.path.normpath(os.path.join(current_wd, component["subdir"]))
        utl.CreateDirectory(temp_wd)
        os.chdir(temp_wd)
        if CURRENT_OS == "macos" and component["name"] == "Helper":
            # Update the team ID in the helper's plist.  xcodebuild won't do it for us as we are embedding the
            # plist via the Other Linker Flags section of the Xcode project.
            update_team_id(os.path.join(pathhelper.ROOT_DIR, component["subdir"], "helper-info.plist"))

        generate_cmd = ["cmake", os.path.join(pathhelper.ROOT_DIR, component["subdir"], "CMakeLists.txt")]
        generate_cmd.extend(["--no-warn-unused-cli", "-DCMAKE_BUILD_TYPE=" + ("Debug" if arghelper.build_debug() else "Release")])
        VCPKG_ROOT = os.getenv('VCPKG_ROOT')
        generate_cmd.extend([f"-DCMAKE_TOOLCHAIN_FILE={VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"])

        vcpkg_triplets_dir = os.path.join(pathhelper.TOOLS_DIR, "vcpkg", "triplets")
        generate_cmd.extend([f"-DVCPKG_OVERLAY_TRIPLETS=\'{vcpkg_triplets_dir}\'"])
        generate_cmd.extend([f"-DVCPKG_TARGET_TRIPLET={CURRENT_VCPKG_TRIPLET}"])

        if arghelper.sign() or arghelper.sign_app() or arghelper.ci_mode():
            generate_cmd.extend(["-DDEFINE_USE_SIGNATURE_CHECK_MACRO=ON"])
        if arghelper.build_tests():
            generate_cmd.extend(["-DIS_BUILD_TESTS=ON"])
        if arghelper.build_cli_only() and CURRENT_OS == "linux":
            generate_cmd.extend(["-DDEFINE_CLI_ONLY_MACRO=ON"])
        if arghelper.static_analysis():
            # Excluding clang-analyzer-optin.cplusplus.VirtualCall because it finds issues in boost etc and it can't be disabled.
            # Also, many QObjects in the GUI may update positions etc in the constructor, which also triggers this.
            # Excluding clang-diagnostic-deprecated-declarations because these warnings are already printed by the compiler.
            # Excluding clang-analyzer-osx.cocoa.RetainCount because clang-tidy does not understand ARC
            generate_cmd.extend(["-DCMAKE_CXX_CLANG_TIDY=clang-tidy;"
                                 "--checks=-clang-analyzer-optin.cplusplus.VirtualCall,-clang-diagnostic-deprecated-declarations,-clang-analyzer-osx.cocoa.RetainCount;"
                                 "--allow-no-checks;"
                                 "--warnings-as-errors=*"])

        if CURRENT_OS == "macos":
            # Build an universal binary only on CI
            if arghelper.ci_mode():
                generate_cmd.extend(["-DCMAKE_OSX_ARCHITECTURES=\'arm64;x86_64\'"])
                generate_cmd.extend(["-DVCPKG_TARGET_TRIPLET=universal-osx"])
        if "generator" in component:
            generate_cmd.extend(["-G", component["generator"]])
        elif CURRENT_OS == "win32":
            generate_cmd.append('-G Ninja')
            generate_cmd.append("-DCMAKE_GENERATOR:STRING=Ninja")
            if arghelper.target_arm64_arch():
                generate_cmd.append("-DCMAKE_SYSTEM_PROCESSOR:STRING=arm64")
        if component["name"] == "Client":
            try:
                build_id = re.search(r"\d+", proc.ExecuteAndGetOutput(["git", "branch", "--show-current"], env=buildenv, shell=False)).group()
                generate_cmd.extend(["-DDEFINE_USE_BUILD_ID_MACRO=" + build_id])
            except Exception:
                # Not on a development branch, ignore
                pass
        if component["name"] == "InstallerBootstrap":
            installer_name = "Windscribe_{}.exe".format(extractor.app_version(True))
            generate_cmd.append(f"-DWINDSCRIBE_INSTALLER_NAME={installer_name}")

        msg.Info(generate_cmd)
        iutl.RunCommand(generate_cmd, env=buildenv, shell=(CURRENT_OS == "win32"))

        build_cmd = ["cmake", "--build", ".", "--config", "Debug" if arghelper.build_debug() else "Release", "--parallel", str(multiprocessing.cpu_count()), "--"]
        if "xcflags" in component:
            build_cmd.extend(component["xcflags"])
        if CURRENT_OS == "macos":
            build_cmd.extend(["DEVELOPMENT_TEAM={}".format(MAC_DEV_TEAM_ID)])
            if component["name"] == "Installer":
                utl.CopyFile(os.path.join(pathhelper.ROOT_DIR, component["subdir"], "Info.plist"),
                             os.path.join(pathhelper.ROOT_DIR, component["subdir"], "temp_Info.plist"))
                temp_info_plist = os.path.join(pathhelper.ROOT_DIR, component["subdir"], "temp_Info.plist")
                update_version_in_plist(temp_info_plist)
                build_cmd.extend(["INFOPLIST_FILE={}".format(temp_info_plist)])

        msg.Info(build_cmd)
        iutl.RunCommand(build_cmd, env=buildenv, shell=(CURRENT_OS == "win32"))

        if CURRENT_OS == "macos":
            if component["name"] == "Helper":
                # Undo what UpdateTeamID did above so version control doesn't see the change.
                restore_helper_info_plist(os.path.join(pathhelper.ROOT_DIR, component["subdir"], "helper-info.plist"))
            elif component["name"] == "Installer":
                utl.RemoveFile(temp_info_plist)


def deploy_component(configdata, component_name, buildenv=None, target_name_override=None):
    component = configdata[component_name][CURRENT_OS]
    c_target = component.get("target", None)

    with utl.PushDir() as current_wd:
        temp_wd = os.path.normpath(os.path.join(current_wd, component["subdir"]))
        os.chdir(temp_wd)

        if CURRENT_OS == "macos" and component["name"] in ["CLI", "Client"] or CURRENT_OS == "linux" or CURRENT_OS == "win32":
            target_location = ""
        elif arghelper.build_debug():
            target_location = "Debug"
        else:
            target_location = "Release"

        if CURRENT_OS == "macos" and "deploy" in component and component["deploy"]:
            install_cmd = ["cmake", "--install", ".", "--prefix", BUILD_INSTALLER_FILES]
            iutl.RunCommand(install_cmd, env=buildenv, shell=(CURRENT_OS == "win32"))
            appfullname = os.path.join(BUILD_INSTALLER_FILES, c_target)
            update_version_in_plist(os.path.join(appfullname, "Contents", "Info.plist"))
            update_team_id(os.path.join(appfullname, "Contents", "Info.plist"))
            msg.Info("Applying mac deploy fixes...")
            apply_mac_deploy_fixes(configdata, component_name, c_target, appfullname)
            sign_mac_entitlements(configdata, component_name, c_target, appfullname)

        if CURRENT_OS == "macos" and ("entitlements" in component and component["entitlements"]):
            appfullname = os.path.join(temp_wd, target_location, c_target)
            sign_mac_entitlements(configdata, component_name, c_target, appfullname)

        # Copy output file(s).
        if CURRENT_OS != "macos":
            install_cmd = ["cmake", "--install", ".", "--prefix", BUILD_INSTALLER_FILES]
            iutl.RunCommand(install_cmd, env=buildenv, shell=(CURRENT_OS == "win32"))

        if BUILD_SYMBOL_FILES and "symbols" in component:
            c_symbols = component.get("symbols", None)
            symbols = [c_symbols] if (type(c_symbols) is not list) else c_symbols
            for s in symbols:
                srcfile = os.path.join(temp_wd, target_location, s)
                if os.path.exists(srcfile):
                    dstfile = BUILD_SYMBOL_FILES
                    if "outdir" in component:
                        dstfile = os.path.join(dstfile, component["outdir"])
                    dstfile = os.path.join(dstfile, os.path.basename(s))
                    utl.CopyFile(srcfile, dstfile)


def build_components(configdata, targetlist):
    # Create an environment with compile-related vars.
    buildenv = os.environ.copy()
    if CURRENT_OS == "win32":
        buildenv.update({"MAKEFLAGS": "S"})
        buildenv.update(iutl.GetVisualStudioEnvironment(arghelper.target_arm64_arch()))
        buildenv.update({"CL": "/MP"})
    # Build all components needed.
    for target in targetlist:
        if target not in configdata:
            raise iutl.InstallError("Undefined target: {} (please check \"{}\")".format(target, BUILD_CFG_NAME))
        if CURRENT_OS in configdata[target]:
            build_component(configdata[target][CURRENT_OS], buildenv)
            deploy_component(configdata, target, buildenv)


def pack_symbols():
    msg.Info("Packing symbols...")
    symbols_archive_name = "WindscribeSymbols_{}.zip".format(extractor.app_version(True))
    zf = zipfile.ZipFile(os.path.join(BUILD_SYMBOL_FILES, "..", symbols_archive_name), "w", zipfile.ZIP_DEFLATED)
    skiplen = len(BUILD_SYMBOL_FILES) + 1
    for filename in glob.glob(BUILD_SYMBOL_FILES + os.sep + "**"):
        if os.path.isdir(filename):
            continue
        filenamepartial = filename[skiplen:]
        msg.Print(filenamepartial)
        zf.write(utl.MakeUnicodePath(filename), filenamepartial)
    zf.close()


def sign_executable_win32(configdata, filename_to_sign=None):
    token = os.getenv("WINDOWS_SIGNING_TOKEN_PASSWORD")
    if token is None or token == "":
        msg.Info("No token password found, skipping signing")
        return

    if "signing_cert_win" in configdata and \
            "path_signtool" in configdata["signing_cert_win"] and \
            "path_cert" in configdata["signing_cert_win"] and \
            "timestamp" in configdata["signing_cert_win"]:
        signtool = os.path.join(pathhelper.ROOT_DIR, configdata["signing_cert_win"]["path_signtool"])
        certfile = os.path.join(pathhelper.ROOT_DIR, configdata["signing_cert_win"]["path_cert"])
        timestamp = configdata["signing_cert_win"]["timestamp"]

        processCode = subprocess.run([signtool, "verify", "/pa", filename_to_sign], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if processCode.returncode != 0:
            try:
                iutl.RunCommand([signtool, "sign",
                                 "/fd", "SHA256",
                                 "/td", "SHA256",
                                 "/tr", timestamp,
                                 "/f", certfile,
                                 "/csp", "eToken Base Cryptographic Provider",
                                 "/kc", "[{{" + token + "}}]=" + os.getenv("CONTAINER_NAME"),
                                 "/n", "Windscribe Limited",
                                 filename_to_sign])
            except Exception:
                # Suppress the actual error message as it will contain the command line which contains sensitive info.
                raise iutl.InstallError("Failed to sign the file: {}".format(filename_to_sign))
    else:
        msg.Info("Skip signing. The signing data is not set in YML.")


def sign_app_win32(configdata):
    if not arghelper.sign() and not arghelper.sign_app():
        return

    # Get list of all exe/dll files in the installer dir, we will sign them
    exe_and_dll_files = []
    for file in glob.glob(os.path.join(BUILD_INSTALLER_FILES, "*.exe")):
        exe_and_dll_files.append(file)
    for file in glob.glob(os.path.join(BUILD_INSTALLER_FILES, "*.dll")):
        exe_and_dll_files.append(file)

    # Sign executable and dll files with a certificate.
    msg.Info("Signing executables and dlls...")
    for f in exe_and_dll_files:
        sign_executable_win32(configdata, f)


def prep_installer_win32(configdata):
    # Copy Windows files.
    msg.Info("Copying libs ...")

    if "additional_files" in configdata["deploy_files"]["win32"]["installer"]:
        additional_dir = os.path.join(pathhelper.ROOT_DIR, "installer", "windows", "additional_files")
        copy_files("additional", configdata["deploy_files"]["win32"]["installer"]["additional_files"], additional_dir, BUILD_INSTALLER_FILES)

    copy_libs(configdata, "win32", "installer", BUILD_INSTALLER_FILES)
    if arghelper.target_arm64_arch():
        copy_libs(configdata, "win32_arm64", "installer", BUILD_INSTALLER_FILES)
        if "additional_files" in configdata["deploy_files"]["win32_arm64"]["installer"]:
            additional_dir = os.path.join(pathhelper.ROOT_DIR, "installer", "windows", "additional_files")
            copy_files("additional arm64", configdata["deploy_files"]["win32_arm64"]["installer"]["additional_files"], additional_dir, BUILD_INSTALLER_FILES)
    else:
        copy_libs(configdata, "win32_x86_64", "installer", BUILD_INSTALLER_FILES)
        if "additional_files" in configdata["deploy_files"]["win32_x86_64"]["installer"]:
            additional_dir = os.path.join(pathhelper.ROOT_DIR, "installer", "windows", "additional_files")
            copy_files("additional x86_64", configdata["deploy_files"]["win32_x86_64"]["installer"]["additional_files"], additional_dir, BUILD_INSTALLER_FILES)

    if "common_files" in configdata:
        copy_files("common", configdata["common_files"], pathhelper.COMMON_DIR, BUILD_INSTALLER_FILES)

    # Pack symbols for crashdump analysis.
    pack_symbols()


def build_installer_win32(configdata):
    # Place everything in a 7z archive.
    installer_info = configdata["installer"]["win32"]
    archive_filename = os.path.normpath(os.path.join(pathhelper.ROOT_DIR, installer_info["subdir"], "resources", "windscribe.7z"))
    # Don't delete the archive after the installer is built so we can use it for installer build/testing/debugging during development.
    if os.path.exists(archive_filename):
        utl.RemoveFile(archive_filename)
    msg.Info("Zipping into " + archive_filename)
    ziptool = os.path.join(pathhelper.TOOLS_DIR, "bin", "7z.exe")
    iutl.RunCommand([ziptool, "a", archive_filename, os.path.join(BUILD_INSTALLER_FILES, "*"),
                     "-y", "-bso0", "-bsp2"])

    # Build and sign the installer.
    buildenv = os.environ.copy()
    buildenv.update({"MAKEFLAGS": "S"})
    buildenv.update(iutl.GetVisualStudioEnvironment(arghelper.target_arm64_arch()))
    buildenv.update({"CL": "/MP"})
    build_component(installer_info, buildenv)
    deploy_component(configdata, "installer", buildenv)


def sign_installer_win32(configdata):
    if not arghelper.sign() and not arghelper.sign_installer():
        return

    msg.Info("Signing installer...")
    sign_executable_win32(configdata, os.path.normpath(os.path.join(BUILD_INSTALLER_FILES, configdata["installer"]["win32"]["target"])))


def build_bootstrap_win32(configdata):
    installer_name = "Windscribe_{}.exe".format(extractor.app_version(True))
    installer_info = configdata["installer"]["win32"]
    buildenv = os.environ.copy()
    buildenv.update({"MAKEFLAGS": "S"})
    buildenv.update(iutl.GetVisualStudioEnvironment(arghelper.target_arm64_arch()))
    buildenv.update({"CL": "/MP"})

    # Pack installer exe into 7z archive
    utl.RenameFile(os.path.normpath(os.path.join(BUILD_INSTALLER_FILES, installer_info["target"])),
                   os.path.normpath(os.path.join(BUILD_INSTALLER_BOOTSTRAP_FILES, installer_name)))

    archive_filename = os.path.normpath(os.path.join(pathhelper.ROOT_DIR, configdata["bootstrap"]["win32"]["subdir"], "resources", "windscribeinstaller.7z"))
    if os.path.exists(archive_filename):
        utl.RemoveFile(archive_filename)
    ziptool = os.path.join(pathhelper.TOOLS_DIR, "bin", "7z.exe")
    iutl.RunCommand([ziptool, "a", archive_filename, os.path.join(BUILD_INSTALLER_BOOTSTRAP_FILES, "*"), "-y", "-bso0", "-bsp2"])

    # Build the bootstrapper
    build_component(configdata["bootstrap"]["win32"], buildenv)
    deploy_component(configdata, "bootstrap", buildenv)

    utl.RenameFile(os.path.normpath(os.path.join(BUILD_INSTALLER_FILES, configdata["bootstrap"]["win32"]["target"])),
                   os.path.normpath(os.path.join(BUILD_INSTALLER_FILES, "..", installer_name)))


def sign_bootstrap_win32(configdata):
    if not arghelper.sign() and not arghelper.sign_bootstrap():
        return

    msg.Info("Signing installer bootstrap...")
    sign_executable_win32(configdata, os.path.join(BUILD_INSTALLER_FILES, "..", "Windscribe_{}.exe".format(extractor.app_version(True))))


def build_installer_mac(configdata, build_path):
    if arghelper.notarize():
        msg.Print("Notarizing app...")
        iutl.RunCommand([pathhelper.notarize_script_filename_absolute(), MAC_DEV_TEAM_ID, BUILD_INSTALLER_FILES, "Windscribe"])
    # Place everything in a lzma tarball.
    msg.Info("Zipping...")
    installer_info = configdata["installer"]["macos"]
    arc_path = os.path.join(pathhelper.ROOT_DIR, installer_info["subdir"], "resources", "windscribe.tar.lzma")
    archive_filename = os.path.normpath(arc_path)
    if os.path.exists(archive_filename):
        utl.RemoveFile(archive_filename)
    iutl.RunCommand(["tar", "--lzma", "-cf", archive_filename, "-C", os.path.join(BUILD_INSTALLER_FILES, "Windscribe.app"), "."])
    # Build and sign the installer.
    with utl.PushDir():
        os.chdir(build_path)
        buildenv = os.environ.copy()
    build_component(installer_info, buildenv)
    deploy_component(configdata, "installer", target_name_override="WindscribeInstaller.app")
    if arghelper.notarize():
        msg.Print("Notarizing installer...")
        iutl.RunCommand([pathhelper.notarize_script_filename_absolute(), MAC_DEV_TEAM_ID, BUILD_INSTALLER_FILES, "WindscribeInstaller"])
    # Drop DMG.
    msg.Print("Preparing dmg...")
    dmg_dir = BUILD_INSTALLER_FILES
    with utl.PushDir(dmg_dir):
        iutl.RunCommand(["python3", "-m", "dmgbuild", "-s",
                         pathhelper.ROOT_DIR + "/installer/mac/dmgbuild/dmgbuild_settings.py",
                         "WindscribeInstaller",
                         "WindscribeInstaller.dmg", "-D", "app=WindscribeInstaller.app", "-D",
                         "background=" + pathhelper.ROOT_DIR + "/installer/mac/dmgbuild/installer_background.png"])
        final_installer_name = os.path.normpath(os.path.join(dmg_dir, "Windscribe_{}.dmg"
                                                             .format(extractor.app_version(True))))
    utl.RenameFile(os.path.join(dmg_dir, "WindscribeInstaller.dmg"), final_installer_name)
    utl.RemoveFile(arc_path)


def code_sign_linux(binary_name, binary_dir):
    binary = os.path.join(binary_dir, binary_name)
    binary_base_name = os.path.basename(binary)
    # Skip DGA library signing, if it not exists (to avoid error)
    if binary_base_name == "libdga.so" and not os.path.exists(binary):
        pass
    else:
        signatures_dir = os.path.join(os.path.dirname(binary), "signatures")
        if not os.path.exists(signatures_dir):
            msg.Print("Creating signatures path: " + signatures_dir)
            utl.CreateDirectory(signatures_dir, True)
        signature_file = signatures_dir + "/" + Path(binary).stem + ".sig"
        msg.Info("Signing " + binary + " -> " + signature_file)
        subprocess.run(["openssl", "dgst", "-sign", "/dev/stdin", "-keyform", "PEM", "-sha256", "-out", signature_file, "-binary", binary], input=os.getenv("LINUX_SIGNING_KEY_FORMATTED").encode())


def copy_libs(configdata, platform, target, dst):
    msg.Info("Copying libs ({})...".format(platform))
    if "libs" in configdata["deploy_files"][platform][target]:
        for k, v in configdata["deploy_files"][platform][target]["libs"].items():
            if (arghelper.target_arm64_arch() and CURRENT_OS == "linux"):
                build_libs_dir = "build-libs"
            else:
                build_libs_dir = "build-libs-arm64" if arghelper.target_arm64_arch() else "build-libs"
            lib_root = iutl.GetDependencyBuildRoot(build_libs_dir, k)
            if not lib_root:
                if k == "dga":
                    msg.Info("DGA library not found, skipping...")
                else:
                    raise iutl.InstallError("Library \"{}\" is not installed.".format(k))
            else:
                copy_files(k, v, lib_root, dst)


def build_installer_linux(configdata):
    # Creates the following:
    # * windscribe_2.x.y_amd64.deb
    # * windscribe_2.x.y_x86_64.rpm

    if arghelper.build_cli_only():
        build_config = "cli"
    else:
        build_config = "gui"

    copy_libs(configdata, "linux", "installer_" + build_config, BUILD_INSTALLER_FILES)

    msg.Info("Fixing rpaths...")
    if "fix_rpath" in configdata["deploy_files"]["linux"]["installer_" + build_config]:
        for k in configdata["deploy_files"]["linux"]["installer_" + build_config]["fix_rpath"]:
            dstfile = os.path.join(BUILD_INSTALLER_FILES, k)
            fix_rpath_linux(dstfile)

    # sign supplementary binaries and move the signatures into InstallerFiles/signatures
    if arghelper.sign() and "linux" in configdata["codesign_files"]:
        for binary_name in configdata["codesign_files"]["linux"]:
            code_sign_linux(binary_name, BUILD_INSTALLER_FILES)

    if "common_files" in configdata:
        copy_files("common", configdata["common_files"], pathhelper.COMMON_DIR, BUILD_INSTALLER_FILES)

    # create .deb with dest_package
    if arghelper.build_deb():
        msg.Info("Creating .deb package...")

        src_package_path = os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "common")
        deb_files_path = os.path.join(pathhelper.ROOT_DIR, "installer", "linux", build_config, "debian_package")

        if platform.machine() == "x86_64":
            if arghelper.build_cli_only():
                dest_package_name = "windscribe-cli_{}_amd64".format(extractor.app_version(True))
            else:
                dest_package_name = "windscribe_{}_amd64".format(extractor.app_version(True))
        elif platform.machine() == "aarch64":
            if arghelper.build_cli_only():
                dest_package_name = "windscribe-cli_{}_arm64".format(extractor.app_version(True))
            else:
                dest_package_name = "windscribe_{}_arm64".format(extractor.app_version(True))

        dest_package_path = os.path.join(BUILD_INSTALLER_FILES, "..", dest_package_name)

        utl.CopyAllFiles(src_package_path, dest_package_path)
        if arghelper.build_cli_only():
            utl.CopyAllFiles(os.path.join(src_package_path, "..", "cli", "overlay"), dest_package_path)
        else:
            utl.CopyAllFiles(os.path.join(src_package_path, "..", "gui", "overlay"), dest_package_path)
        utl.CopyAllFiles(deb_files_path, dest_package_path)
        utl.CopyAllFiles(BUILD_INSTALLER_FILES, os.path.join(dest_package_path, "opt", "windscribe"))

        update_version_in_config(os.path.join(dest_package_path, "DEBIAN", "control"))
        update_arch_in_config(os.path.join(dest_package_path, "DEBIAN", "control"))
        iutl.RunCommand(["fakeroot", "dpkg-deb", "--build", dest_package_path])

    for distro in ["fedora", "opensuse"]:
        if arghelper.build_rpm(distro):
            build_rpm(distro, build_config)


def build_rpm(distro, build_config):
    msg.Info("Creating {} .rpm package...".format(distro))

    utl.CopyAllFiles(os.path.join(pathhelper.ROOT_DIR, "installer", "linux", build_config, "rpm_{}_package".format(distro)), os.path.join(pathlib.Path.home(), "rpmbuild"))
    utl.CopyAllFiles(os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "common"), os.path.join(pathlib.Path.home(), "rpmbuild", "SOURCES"))

    if arghelper.build_cli_only():
        utl.CopyAllFiles(os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "cli", "overlay"), os.path.join(pathlib.Path.home(), "rpmbuild", "SOURCES"))
    else:
        utl.CopyAllFiles(os.path.join(pathhelper.ROOT_DIR, "installer", "linux", "gui", "overlay"), os.path.join(pathlib.Path.home(), "rpmbuild", "SOURCES"))

    utl.CopyAllFiles(BUILD_INSTALLER_FILES, os.path.join(pathlib.Path.home(), "rpmbuild", "SOURCES", "opt", "windscribe"))

    update_version_in_config(os.path.join(pathlib.Path.home(), "rpmbuild", "SPECS", "windscribe_rpm.spec"))
    iutl.RunCommand(["rpmbuild", "-bb", os.path.join(pathlib.Path.home(), "rpmbuild", "SPECS", "windscribe_rpm.spec")])

    # We use 'arm64' while rpmbuild uses 'aarch64'
    arch1 = "x86_64" if platform.machine() == "x86_64" else "arm64"
    arch2 = "x86_64" if platform.machine() == "x86_64" else "aarch64"

    utl.CopyFile(os.path.join(pathlib.Path.home(), "rpmbuild", "RPMS", arch2, "windscribe{}-{}-0.{}.rpm".format("-cli" if arghelper.build_cli_only() else "", extractor.app_version(False), arch2)),
                 os.path.join(BUILD_INSTALLER_FILES, "..", "windscribe{}_{}_{}_{}.rpm".format("-cli" if arghelper.build_cli_only() else "", extractor.app_version(True), arch1, distro)))


def update_vcpkg_dependencies():
    global CURRENT_VCPKG_TRIPLET

    if CURRENT_OS == "macos":
        if arghelper.ci_mode():
            # Build an universal binary only on CI
            CURRENT_VCPKG_TRIPLET = "universal-osx"
        elif platform.machine() == "x86_64":
            CURRENT_VCPKG_TRIPLET = "x64-osx"
        else:
            CURRENT_VCPKG_TRIPLET = "arm64-osx"

    elif CURRENT_OS == "win32":
        # We build only release dependencies on CI
        if arghelper.ci_mode():
            if arghelper.target_arm64_arch():
                CURRENT_VCPKG_TRIPLET = "arm64-windows-static-release"
            else:
                CURRENT_VCPKG_TRIPLET = "x64-windows-static-release"
        # On developers computers we compile both the Debug and Release versions with standard x64-windows-static triplet
        else:
            CURRENT_VCPKG_TRIPLET = "x64-windows-static"
    elif CURRENT_OS == "linux":
        if arghelper.target_arm64_arch():
            CURRENT_VCPKG_TRIPLET = "arm64-linux"
        else:
            CURRENT_VCPKG_TRIPLET = "x64-linux"

    VCPKG_ROOT = os.getenv('VCPKG_ROOT')
    cmd = [f"{VCPKG_ROOT}/vcpkg", "install", f"--x-install-root={VCPKG_ROOT}/installed", "--x-manifest-root=" + os.path.join(pathhelper.TOOLS_DIR, "vcpkg"), "--overlay-triplets=" + os.path.join(pathhelper.TOOLS_DIR, "vcpkg", "triplets")]

    cmd.extend([f"--triplet={CURRENT_VCPKG_TRIPLET}"])
    if CURRENT_OS == "win32":
        # We're building all(X64 and ARM64) Windows architectures on X64
        if arghelper.ci_mode():
            cmd.extend(["--host-triplet=x64-windows-static-release"])
        else:
            cmd.extend(["--host-triplet=x64-windows-static"])
    elif CURRENT_OS == "linux":
        cmd.extend([f"--host-triplet={CURRENT_VCPKG_TRIPLET}"])

    cmd.extend(["--clean-buildtrees-after-build"])
    cmd.extend(["--clean-packages-after-build"])

    iutl.RunCommand(cmd)


def build_all():
    msg.Print("Building {}...".format(BUILD_TITLE))

    # Load config.
    configdata = utl.LoadConfig(os.path.join(pathhelper.TOOLS_DIR, "{}".format(BUILD_CFG_NAME)))
    if not configdata:
        raise iutl.InstallError("Failed to load config \"{}\".".format(BUILD_CFG_NAME))
    if "targets" not in configdata:
        raise iutl.InstallError("No {} targets defined in \"{}\".".format(CURRENT_OS, BUILD_CFG_NAME))

    msg.Info("App version extracted: \"{}\"".format(extractor.app_version(True)))

    if arghelper.build_app() or arghelper.build_installer() or arghelper.build_bootstrap():
        # Only update deps if building something
        update_vcpkg_dependencies()

    if CURRENT_OS == "win32":
        # Verify Visual Studio is good-to-go.
        buildenv = os.environ.copy()
        buildenv.update(iutl.GetVisualStudioEnvironment(arghelper.target_arm64_arch()))

    # Prepare output.
    artifact_dir = os.path.join(pathhelper.ROOT_DIR, "build-exe")
    if arghelper.clean():
        utl.RemoveDirectory(artifact_dir)
    if CURRENT_OS == "win32":
        temp_dir = iutl.PrepareTempDirectory("installer" + ("-arm64" if arghelper.target_arm64_arch() else ""), not arghelper.ci_mode())
        build_dir = os.path.join(pathhelper.ROOT_DIR, "build" + ("-arm64" if arghelper.target_arm64_arch() else ""))
    else:
        temp_dir = iutl.PrepareTempDirectory("installer", not arghelper.ci_mode())
        build_dir = os.path.join(pathhelper.ROOT_DIR, "build")
    utl.CreateDirectory(build_dir, arghelper.clean())
    global BUILD_INSTALLER_FILES, BUILD_TEST_FILES, BUILD_SYMBOL_FILES, BUILD_INSTALLER_BOOTSTRAP_FILES
    BUILD_INSTALLER_FILES = os.path.join(temp_dir, "InstallerFiles")
    BUILD_TEST_FILES = os.path.join(build_dir, "test")
    BUILD_INSTALLER_BOOTSTRAP_FILES = os.path.join(temp_dir, "InstallerBootstrapFiles")
    utl.CreateDirectory(BUILD_INSTALLER_FILES, not arghelper.ci_mode())
    utl.CreateDirectory(BUILD_INSTALLER_BOOTSTRAP_FILES, not arghelper.ci_mode())
    if CURRENT_OS == "win32":
        BUILD_SYMBOL_FILES = os.path.join(temp_dir, "SymbolFiles")
        utl.CreateDirectory(BUILD_SYMBOL_FILES, not arghelper.ci_mode())

    # Build the components.
    with utl.PushDir(build_dir):
        if arghelper.build_app():
            build_components(configdata, configdata["targets"])
            if (CURRENT_OS == "win32"):
                prep_installer_win32(configdata)
                if arghelper.build_tests():
                    copy_files("Test", ["wsnet_test.exe"], os.path.join(build_dir, "client"), BUILD_TEST_FILES)
        if arghelper.sign_app():
            if (CURRENT_OS == "win32"):
                sign_app_win32(configdata)
        if CURRENT_OS == "win32":
            if arghelper.build_installer():
                build_installer_win32(configdata)
            if arghelper.sign_installer():
                sign_installer_win32(configdata)
            if arghelper.build_bootstrap():
                build_bootstrap_win32(configdata)
                if (not arghelper.sign_bootstrap()):
                    install_artifacts(configdata, artifact_dir, temp_dir)
            if arghelper.sign_bootstrap():
                sign_bootstrap_win32(configdata)
                install_artifacts(configdata, artifact_dir, temp_dir)
        elif CURRENT_OS == "macos":
            if arghelper.build_installer():
                build_installer_mac(configdata, build_dir)
                install_artifacts(configdata, artifact_dir, temp_dir)
        elif CURRENT_OS == "linux":
            if arghelper.build_installer():
                build_installer_linux(configdata)
                install_artifacts(configdata, artifact_dir, temp_dir)


def install_artifacts(configdata, artifact_dir, temp_dir):
    # Copy artifacts.
    msg.Print("Installing artifacts...")
    utl.CreateDirectory(artifact_dir, True)
    if CURRENT_OS == "macos":
        artifact_path = BUILD_INSTALLER_FILES
        installer_info = configdata["installer"]["macos"]
        if "outdir" in installer_info:
            artifact_path = os.path.join(artifact_path, installer_info["outdir"])
    else:
        artifact_path = temp_dir
    for filename in glob.glob(artifact_path + os.sep + "*"):
        if os.path.isdir(filename):
            continue
        filetitle = os.path.basename(filename)
        utl.CopyFile(filename, os.path.join(artifact_dir, filetitle))
        msg.HeadPrint("Ready: \"{}\"".format(filetitle))

    # Cleanup.
    if not arghelper.ci_mode():
        msg.Print("Cleaning temporary directory...")
        utl.RemoveDirectory(temp_dir)


def prechecks():
    # check if the VPKG_ROOT environment variable exists
    if (arghelper.build_app() or arghelper.build_installer() or arghelper.build_bootstrap()) and "VCPKG_ROOT" not in os.environ:
        raise IOError("The VCPKG_ROOT environment variable is not set")

    # USER ERROR - Cannot notarize without signing
    if arghelper.notarize() and not arghelper.sign():
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
        msg.Info("Using signing identity - " + MAC_DEV_ID_KEY_NAME)

    if arghelper.sign():
        # on linux we need keypair to sign -- check that they exist in the correct location
        if CURRENT_OS == utl.CURRENT_OS_LINUX:
            pubkey = pathhelper.linux_public_key_filename_absolute()
            if not os.path.exists(pubkey) or os.getenv("LINUX_SIGNING_KEY_FORMATTED") is None or os.getenv("LINUX_SIGNING_KEY_FORMATTED") == "":
                raise IOError("Code signing (--sign) is enabled but key.pub and/or key.pem were not found.")
            generate_include_file_from_pub_key(pathhelper.linux_include_key_filename_absolute(), pubkey)

        # early check for cert password on windows
        if CURRENT_OS == utl.CURRENT_OS_WIN:
            if (os.getenv('WINDOWS_SIGNING_TOKEN_PASSWORD') is None or os.getenv('WINDOWS_SIGNING_TOKEN_PASSWORD') == ""):
                raise IOError("Cannot sign without token password. Please set WINDOWS_SIGNING_TOKEN_PASSWORD environment variable.")


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


# main script logic, and timing display
if __name__ == "__main__":
    start_time = time.time()
    CURRENT_OS = utl.GetCurrentOS()

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
        elif arghelper.build_mode():
            prechecks()
            build_all()
        else:
            if arghelper.clean_only():
                msg.Print("Cleaning...")
                clean_all_temp_and_build_dirs()
        exitcode = 0
    except iutl.InstallError as e:
        msg.Error(e)
        exitcode = e.exitcode
    except IOError as e:
        msg.Error(e)
        exitcode = 1

    # timing output
    if exitcode == 0:
        elapsed_time = time.time() - start_time
        if elapsed_time >= 60:
            msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time / 60, elapsed_time % 60))
        else:
            msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)
    sys.exit(exitcode)
