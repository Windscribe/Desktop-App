#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2024, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# purpose: to help with program arguments

class ArgHelper:
    # general
    OPTION_HELP = "--help"
    OPTION_CLEAN_ONLY = "--clean-only"
    OPTION_DEBUG = "--debug"
    # signing
    OPTION_NOTARIZE = "--notarize"
    OPTION_SIGN = "--sign"
    # partial builds
    OPTION_BUILD_APP = "--build-app"
    OPTION_SIGN_APP = "--sign-app"
    OPTION_BUILD_INSTALLER = "--build-installer"
    OPTION_SIGN_INSTALLER = "--sign-installer"
    OPTION_BUILD_BOOTSTRAP = "--build-bootstrap"
    OPTION_SIGN_BOOTSTRAP = "--sign-bootstrap"
    OPTION_BUILD_TESTS = "--build-tests"
    OPTION_STATIC_ANALYSIS = "--static-analysis"
    OPTION_CLEAN = "--clean"
    # CI-specific options
    OPTION_CI_MODE = "--ci-mode"
    # linux packaging
    OPTION_BUILD_RPM = "--build-rpm"
    OPTION_BUILD_RPM_OPENSUSE = "--build-rpm-opensuse"
    OPTION_BUILD_DEB = "--build-deb"
    OPTION_BUILD_CLI_ONLY = "--build-cli-only"
    # target architecture
    OPTION_TARGET_X86_64 = "--x86_64"  # this is the default
    OPTION_TARGET_ARM64 = "--arm64"

    options = list()
    options.append(("\nGeneral", ""))
    options.append((OPTION_HELP, "Prints this help menu"))
    options.append((OPTION_CLEAN_ONLY, "Cleans the temporary files created during building"))
    options.append((OPTION_DEBUG, "Build project as debug"))
    options.append(("\nSigning", ""))
    options.append((OPTION_NOTARIZE, "Notarizes the app after building (Mac only, CI-only, requires --sign)"))
    options.append((OPTION_SIGN, "Sign the build"))
    options.append(("\nBuild specific components", ""))
    options.append((OPTION_BUILD_APP, "Build the app. On MacOS & Linux, also signs."))
    options.append((OPTION_SIGN_APP, "Sign the app (Windows only)"))
    options.append((OPTION_BUILD_INSTALLER, "Build the installer. On MacOS, also signs."))
    options.append((OPTION_SIGN_INSTALLER, "Sign the installer (Windows only)"))
    options.append((OPTION_BUILD_BOOTSTRAP, "Build the bootstrap (Windows only)"))
    options.append((OPTION_SIGN_BOOTSTRAP, "Sign the bootstrap (Windows only)"))
    options.append((OPTION_BUILD_TESTS, "Build tests (only has an effect if --build-app is also used)"))
    options.append((OPTION_STATIC_ANALYSIS, "Run static analysis on the code"))
    options.append((OPTION_CLEAN, "Fully clean previous build files before building"))
    options.append(("\nCI-specific options", ""))
    options.append((OPTION_CI_MODE, "Used to indicate app is building on CI"))
    options.append(("\nLinux packaging", ""))
    options.append((OPTION_BUILD_RPM, "Build .rpm package for Red Hat and derivative distros"))
    options.append((OPTION_BUILD_RPM_OPENSUSE, "Build .rpm package for OpenSUSE and derivative distros"))
    options.append((OPTION_BUILD_DEB, "Build .deb package for Debian and derivative distros (default)"))
    options.append((OPTION_BUILD_CLI_ONLY, "Build package without GUI (Linux only)"))
    options.append(("\nTarget Architecture", ""))
    options.append((OPTION_TARGET_X86_64, "Build for the Intel/AMD 64-bit x86 CPU architecture (default) (Windows only)"))
    options.append((OPTION_TARGET_ARM64, "Build for the ARM 64-bit CPU architecture (Windows only)"))

    def __init__(self, program_arg_list):
        self.args_only = program_arg_list[1:]

        # 4 main branches
        self.mode_clean_only = ArgHelper.OPTION_CLEAN_ONLY in program_arg_list
        self.mode_help = ArgHelper.OPTION_HELP in program_arg_list
        self.mode_debug = ArgHelper.OPTION_DEBUG in program_arg_list

        # building
        self.mode_clean = ArgHelper.OPTION_CLEAN in program_arg_list
        self.mode_build_app = ArgHelper.OPTION_BUILD_APP in program_arg_list
        self.mode_sign_app = ArgHelper.OPTION_SIGN_APP in program_arg_list
        self.mode_build_installer = ArgHelper.OPTION_BUILD_INSTALLER in program_arg_list
        self.mode_sign_installer = ArgHelper.OPTION_SIGN_INSTALLER in program_arg_list
        self.mode_build_bootstrap = ArgHelper.OPTION_BUILD_BOOTSTRAP in program_arg_list
        self.mode_sign_bootstrap = ArgHelper.OPTION_SIGN_BOOTSTRAP in program_arg_list
        self.mode_build_tests = ArgHelper.OPTION_BUILD_TESTS in program_arg_list
        self.mode_static_analysis = ArgHelper.OPTION_STATIC_ANALYSIS in program_arg_list
        # if nothing specified, build everything
        if (not self.mode_build_app and not self.mode_sign_app and not self.mode_build_installer and not self.mode_sign_installer and not self.mode_build_bootstrap and not self.mode_sign_bootstrap):
            self.mode_build_app = True
            self.mode_build_installer = True
            self.mode_build_bootstrap = True
            if (ArgHelper.OPTION_SIGN in program_arg_list):
                self.mode_sign_app = True
                self.mode_sign_installer = True
                self.mode_sign_bootstrap = True

        # signing related
        self.mode_sign = ArgHelper.OPTION_SIGN in program_arg_list
        self.mode_notarize = ArgHelper.OPTION_NOTARIZE in program_arg_list

        # CI related
        self.mode_ci = ArgHelper.OPTION_CI_MODE in program_arg_list

        # linux packaging
        no_packaging_selected = ArgHelper.OPTION_BUILD_RPM not in program_arg_list and \
            ArgHelper.OPTION_BUILD_RPM_OPENSUSE not in program_arg_list and \
            ArgHelper.OPTION_BUILD_DEB not in program_arg_list

        self.mode_build_deb = no_packaging_selected or ArgHelper.OPTION_BUILD_DEB in program_arg_list
        self.mode_build_rpm = ArgHelper.OPTION_BUILD_RPM in program_arg_list
        self.mode_build_rpm_opensuse = ArgHelper.OPTION_BUILD_RPM_OPENSUSE in program_arg_list
        self.mode_build_cli_only = ArgHelper.OPTION_BUILD_CLI_ONLY in program_arg_list

        # target architecture
        no_arch_selected = ArgHelper.OPTION_TARGET_X86_64 not in program_arg_list and ArgHelper.OPTION_TARGET_ARM64 not in program_arg_list
        self.target_x86_64 = no_arch_selected or ArgHelper.OPTION_TARGET_X86_64 in program_arg_list
        self.target_arm64 = ArgHelper.OPTION_TARGET_ARM64 in program_arg_list

    def ci_mode(self):
        return self.mode_ci

    def clean_only(self):
        return self.mode_clean_only

    def clean(self):
        return self.mode_clean

    def build_app(self):
        return self.mode_build_app

    def build_installer(self):
        return self.mode_build_installer

    def build_bootstrap(self):
        return self.mode_build_bootstrap

    def build_tests(self):
        return self.mode_build_tests

    def static_analysis(self):
        return self.mode_static_analysis

    def sign_app(self):
        return self.mode_sign_app

    def sign_installer(self):
        return self.mode_sign_installer

    def sign_bootstrap(self):
        return self.mode_sign_bootstrap

    def sign(self):
        return self.mode_sign

    def notarize(self):
        return self.mode_notarize

    def build_mode(self):
        return not self.mode_help and not self.mode_clean_only

    def help(self):
        return self.mode_help

    def build_rpm(self, distro):
        if (distro == "opensuse"):
            return self.mode_build_rpm_opensuse
        return self.mode_build_rpm

    def build_deb(self):
        return self.mode_build_deb

    def build_cli_only(self):
        return self.mode_build_cli_only

    def build_debug(self):
        return self.mode_debug

    def target_x86_64_arch(self):
        return self.target_x86_64

    def target_arm64_arch(self):
        return self.target_arm64

    def invalid_mode(self):
        if self.mode_help:
            args = self.args_only[:]
            args.remove(ArgHelper.OPTION_HELP)
            if len(args) > 0:
                return "Cannot use help with any other argument"
        if self.mode_clean_only:
            args = self.args_only[:]
            if args.count(ArgHelper.OPTION_CLEAN_ONLY) > 0:
                args.remove(ArgHelper.OPTION_CLEAN_ONLY)
            if len(args) > 0:
                return "Cannot use clean-only with any other argument"
        return ""
