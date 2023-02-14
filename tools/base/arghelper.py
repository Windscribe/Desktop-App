#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# purpose: to help with program arguments

import sys


class ArgHelper:
    # general
    OPTION_HELP = "--help"
    OPTION_CLEAN_ONLY = "--clean-only"
    OPTION_DELETE_SECRETS_ONLY = "--delete-secrets-only"
    OPTION_DOWNLOAD_SECRETS = "--download-secrets"
    # signing
    OPTION_NOTARIZE = "--notarize"
    OPTION_SIGN = "--sign"
    # secrets
    OPTION_ONE_PASSWORD_USER = "--op-user"
    OPTION_ONE_PASSWORD_SESSION = "--session"
    OPTION_USE_LOCAL_SECRETS = "--use-local-secrets"
    # partial builds
    OPTION_NO_CLEAN = "--no-clean" # basically exists to achieve partial builds
    OPTION_NO_APP = "--no-app"
    OPTION_NO_COM = "--no-com"
    OPTION_NO_INSTALLER = "--no-installer"
    # CI-specific options
    OPTION_CI_MODE = "--ci-mode"
    # linux packaging
    OPTION_BUILD_RPM = "--build-rpm"
    OPTION_BUILD_DEB = "--build-deb"

    options = list()
    options.append(("\nGeneral", ""))
    options.append((OPTION_HELP, "Prints this help menu"))
    options.append((OPTION_CLEAN_ONLY, "Cleans the temporary files created during building"))
    options.append((OPTION_DELETE_SECRETS_ONLY, "Deletes secrets needed for signing"))
    options.append((OPTION_DOWNLOAD_SECRETS, "Download secrets from 1password (not recommended)"))
    options.append((OPTION_NO_INSTALLER, "Do not build the installer component"))
    options.append(("\nSigning", ""))
    options.append((OPTION_NOTARIZE, "Notarizes the app after building (Mac only, CI-only, requires --sign)"))
    options.append((OPTION_SIGN, "Sign the build"))
    options.append(("\nSecrets", ""))
    options.append((OPTION_ONE_PASSWORD_USER, "1password user for windscribe employee secret download"))
    options.append((OPTION_ONE_PASSWORD_SESSION, "1password session to skip 1p login"))
    options.append((OPTION_USE_LOCAL_SECRETS, "Do not download secrets, use local secrets"))
    options.append(("\nComponent skipping", ""))
    options.append((OPTION_NO_CLEAN, "Skip cleaning up temp files/folders. Helps speed up re-building"))
    options.append((OPTION_NO_APP, "Do not build the main application components"))
    options.append((OPTION_NO_COM, "Do not build the COM components required for auth helper"))
    options.append((OPTION_NO_INSTALLER, "Do not build the installer component"))
    options.append(("\nCI-specific options", ""))
    options.append((OPTION_CI_MODE, "Used to indicate app is building on CI"))
    options.append(("\nLinux packaging", ""))
    options.append((OPTION_BUILD_RPM, "Build .rpm package for Red Hat and derivative distros"))
    options.append((OPTION_BUILD_DEB, "Build .deb package for Debian and derivative distros (default)"))

    def __init__(self, program_arg_list):
        self.args_only = program_arg_list[1:]

        # 4 main branches
        self.mode_clean_only = ArgHelper.OPTION_CLEAN_ONLY in program_arg_list
        self.mode_help = ArgHelper.OPTION_HELP in program_arg_list
        self.mode_delete_secrets = ArgHelper.OPTION_DELETE_SECRETS_ONLY in program_arg_list
        self.mode_download_secrets = ArgHelper.OPTION_DOWNLOAD_SECRETS in program_arg_list

        # building
        self.mode_post_clean = not (ArgHelper.OPTION_NO_CLEAN in program_arg_list)
        self.mode_build_app = not (ArgHelper.OPTION_NO_APP in program_arg_list)
        self.mode_build_com = not (ArgHelper.OPTION_NO_COM in program_arg_list)
        self.mode_build_installer = not (ArgHelper.OPTION_NO_INSTALLER in program_arg_list)

        # signing related
        self.mode_sign = ArgHelper.OPTION_SIGN in program_arg_list
        self.mode_notarize = ArgHelper.OPTION_NOTARIZE in program_arg_list
        self.mode_use_local_secrets = ArgHelper.OPTION_USE_LOCAL_SECRETS in program_arg_list

        # CI related
        self.mode_ci = ArgHelper.OPTION_CI_MODE in program_arg_list

        # linux packaging
        no_packaging_selected = ArgHelper.OPTION_BUILD_RPM not in program_arg_list and ArgHelper.OPTION_BUILD_DEB not in program_arg_list

        self.mode_build_deb = no_packaging_selected or ArgHelper.OPTION_BUILD_DEB in program_arg_list
        self.mode_build_rpm = ArgHelper.OPTION_BUILD_RPM in program_arg_list

        # 1password user/session
        try:
            user_index = program_arg_list.index(ArgHelper.OPTION_ONE_PASSWORD_USER) + 1
            if user_index >= len(sys.argv):
                raise IOError()
            self.one_password_user = program_arg_list[user_index]

            try:
                session_token_index = program_arg_list.index(ArgHelper.OPTION_ONE_PASSWORD_SESSION) + 1
                if session_token_index >= len(program_arg_list):
                    raise IOError()
                self.one_password_session = program_arg_list[session_token_index]
            except:
                self.one_password_session = ""
        except:
            self.one_password_user = ""
            self.one_password_session = ""

    def ci_mode(self):
        return self.mode_ci

    def clean_only(self):
        return self.mode_clean_only

    def post_clean(self):
        return self.mode_post_clean

    def build_app(self):
        return self.mode_build_app

    def build_com(self):
        return self.mode_build_com

    def build_installer(self):
        return self.mode_build_installer

    def sign_app(self):
        return self.mode_sign

    def notarize(self):
        return self.mode_notarize

    def one_password_username(self):
        return self.one_password_user

    def valid_one_password_username(self):
        return self.one_password_user != ""

    def one_password_session_token(self):
        return self.one_password_session

    def use_local_secrets(self):
        return self.mode_use_local_secrets

    def build_mode(self):
        return not self.mode_help and not self.mode_clean_only and not self.mode_delete_secrets

    def help(self):
        return self.mode_help

    def delete_secrets_only(self):
        return self.mode_delete_secrets

    def download_secrets(self):
        return self.mode_download_secrets

    def build_rpm(self):
        return self.mode_build_rpm

    def build_deb(self):
        return self.mode_build_deb

    def invalid_mode(self):
        if self.mode_help:
            args = self.args_only[:]
            args.remove(ArgHelper.OPTION_HELP)
            if len(args) > 0:
                return "Cannot use help with any other argument"
        if self.mode_clean_only or self.mode_delete_secrets:
            args = self.args_only[:]
            if args.count(ArgHelper.OPTION_CLEAN_ONLY) > 0:
                args.remove(ArgHelper.OPTION_CLEAN_ONLY)
            if args.count(ArgHelper.OPTION_DELETE_SECRETS_ONLY):
                args.remove(ArgHelper.OPTION_DELETE_SECRETS_ONLY)
            if len(args) > 0:
                return "Cannot use clean-only and delete-secrets-only with any other argument"
        return ""
