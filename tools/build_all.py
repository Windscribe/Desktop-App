#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System - CMake Wrapper
# Copyright (c) 2020-2025, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: Thin wrapper around CMake for building Windscribe
import multiprocessing
import os
import shutil
import sys
import time

# To ensure modules in the 'base' folder can import other modules in base.
import base.pathhelper as pathhelper
sys.path.append(pathhelper.BASE_DIR)

import base.messages as msg
import base.utils as utl
import deps.installutils as iutl
from base.arghelper import ArgHelper

# Windscribe settings
CURRENT_OS = ""

arghelper = ArgHelper(sys.argv)


def get_cmake_configure_args():
    """Build CMake configure arguments from command line options."""
    args = []

    # Build type
    args.append(f"-DCMAKE_BUILD_TYPE={'Debug' if arghelper.build_debug() else 'Release'}")

    # Build options
    args.append(f"-DBUILD_APP={'ON' if arghelper.build_app() else 'OFF'}")
    args.append(f"-DBUILD_INSTALLER={'ON' if arghelper.build_installer() else 'OFF'}")
    args.append(f"-DBUILD_TESTS={'ON' if arghelper.build_tests() else 'OFF'}")
    args.append(f"-DSTATIC_ANALYSIS={'ON' if arghelper.static_analysis() else 'OFF'}")
    args.append(f"-DCI_MODE={'ON' if arghelper.ci_mode() else 'OFF'}")

    # Signing options
    args.append(f"-DSIGN_APP={'ON' if arghelper.sign_app() else 'OFF'}")
    args.append(f"-DSIGN_INSTALLER={'ON' if arghelper.sign_installer() else 'OFF'}")
    args.append(f"-DSIGN_BOOTSTRAP={'ON' if arghelper.sign_bootstrap() else 'OFF'}")
    args.append(f"-DENABLE_NOTARIZE={'ON' if arghelper.notarize() else 'OFF'}")

    # Platform-specific options
    if CURRENT_OS == "win32":
        args.append(f"-DBUILD_BOOTSTRAP={'ON' if arghelper.build_bootstrap() else 'OFF'}")
        args.append(f"-DBUILD_ARM64={'ON' if arghelper.target_arm64_arch() else 'OFF'}")
    elif CURRENT_OS == "linux":
        args.append(f"-DBUILD_CLI_ONLY={'ON' if arghelper.build_cli_only() else 'OFF'}")
        args.append(f"-DBUILD_DEB={'ON' if arghelper.build_deb() else 'OFF'}")
        args.append(f"-DBUILD_RPM={'ON' if arghelper.build_rpm('fedora') else 'OFF'}")
        args.append(f"-DBUILD_RPM_OPENSUSE={'ON' if arghelper.build_rpm('opensuse') else 'OFF'}")

    return args


def get_build_environment():
    """Get environment variables for building."""
    buildenv = os.environ.copy()
    vcpkg_root = os.getenv('VCPKG_ROOT')
    if CURRENT_OS == "win32":
        buildenv.update({"MAKEFLAGS": "S"})
        buildenv.update(iutl.GetVisualStudioEnvironment(arghelper.target_arm64_arch()))
        buildenv.update({"CL": "/MP"})
        if vcpkg_root:
            buildenv["VCPKG_ROOT"] = vcpkg_root
    return buildenv


def run_cmake_configure(build_dir):
    """Run CMake configure step."""
    msg.Info("Configuring CMake...")

    # Remove CMake cache to allow reconfiguration with different paths
    cache_file = os.path.join(build_dir, "CMakeCache.txt")
    cmake_files_dir = os.path.join(build_dir, "CMakeFiles")

    if os.path.exists(cache_file):
        msg.Info("Removing existing CMake cache...")
        os.remove(cache_file)

    if os.path.exists(cmake_files_dir):
        shutil.rmtree(cmake_files_dir)

    cmake_args = ["cmake", "-S", pathhelper.ROOT_DIR, "-B", build_dir]
    cmake_args.extend(get_cmake_configure_args())

    if CURRENT_OS == "win32":
        cmake_args.extend(["-G", "Ninja"])

    iutl.RunCommand(cmake_args, env=get_build_environment())


def run_cmake_build(build_dir):
    """Run CMake build step."""
    num_jobs = multiprocessing.cpu_count()
    buildenv = get_build_environment()

    msg.Info(f"Building with {num_jobs} parallel jobs...")
    iutl.RunCommand(["cmake", "--build", build_dir, "--parallel", str(num_jobs)], env=buildenv)


def clean_build_dirs():
    """Clean build and output directories."""
    msg.Info("Cleaning build directories...")

    utl.RemoveDirectory(os.path.join(pathhelper.ROOT_DIR, "build"))
    utl.RemoveDirectory(os.path.join(pathhelper.ROOT_DIR, "build-exe"))


def cleanup_secrets():
    """Clean up secrets and temporary files after build."""
    msg.Info("Cleaning up secrets and temporary files...")
    utl.RemoveDirectory(pathhelper.TEMP_DIR)
    utl.RemoveDirectory(pathhelper.mac_provision_profile_folder_name_absolute())


def build_all():
    # Determine build directory
    build_dir = os.path.join(pathhelper.ROOT_DIR, "build")

    # Clean if requested
    if arghelper.clean():
        clean_build_dirs()

    # Configure CMake
    run_cmake_configure(build_dir)

    # Build with CMake
    run_cmake_build(build_dir)

    # Clean up secrets if in CI mode
    if arghelper.ci_mode():
        cleanup_secrets()

    msg.HeadPrint("Build completed successfully!")


def print_help_output():
    """Print help information."""
    msg.Print("*" * 60)
    msg.Print("Windscribe build_all help menu")
    msg.Print("*" * 60)
    msg.Print("Windscribe's build_all script is used to build and sign the desktop app for Windows, Mac and Linux.")
    msg.Print("By default it will attempt to build without signing.")
    msg.Print("")

    for name, description in arghelper.options:
        msg.Print(f"{name}: {description}")
    msg.Print("")


# Main script logic
if __name__ == "__main__":
    start_time = time.time()
    CURRENT_OS = utl.GetCurrentOS()

    try:
        if CURRENT_OS not in ["win32", "macos", "linux"]:
            raise IOError(f"Building is not supported on {CURRENT_OS}.")

        # Check for invalid modes
        err = arghelper.invalid_mode()
        if err:
            raise IOError(err)

        # Main decision logic
        if arghelper.help():
            print_help_output()
        elif arghelper.build_mode():
            build_all()
        else:
            if arghelper.clean_only():
                msg.Print("Cleaning...")
                clean_build_dirs()

        exitcode = 0
    except iutl.InstallError as e:
        msg.Error(e)
        exitcode = e.exitcode
    except IOError as e:
        msg.Error(e)
        exitcode = 1

    # Timing output
    if exitcode == 0:
        elapsed_time = time.time() - start_time
        if elapsed_time >= 60:
            msg.HeadPrint(f"All done: {int(elapsed_time / 60)} minutes {int(elapsed_time % 60)} seconds elapsed")
        else:
            msg.HeadPrint(f"All done: {int(elapsed_time)} seconds elapsed")

    sys.exit(exitcode)
