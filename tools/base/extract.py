#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: library for extraction functions

import os
import re
from . import pathhelper


class Extractor:
    # memoize so we don't have to read the file more than once per variable
    extracted_app_version = ""
    extracted_app_version_with_beta_suffix = ""
    extracted_mac_signing_params = ("", "")

    def __init__(self):
        pass

    def app_version(self, include_beta_suffix=False):
        if not include_beta_suffix:
            if not Extractor.extracted_app_version:
                Extractor.extracted_app_version = self.__extract_app_version()
            return Extractor.extracted_app_version
        else:
            if not Extractor.extracted_app_version_with_beta_suffix:
                Extractor.extracted_app_version_with_beta_suffix = self.__extract_app_version(True)
            return Extractor.extracted_app_version_with_beta_suffix

    def mac_signing_params(self):
        if not Extractor.extracted_app_version:
            Extractor.extracted_mac_signing_params = self.__extract_mac_signing_params()
        return Extractor.extracted_mac_signing_params

    @staticmethod
    def __extract_app_version(include_beta_suffix=False):
        version_file = os.path.join(pathhelper.COMMON_DIR, "version", "windscribe_version.h")
        values = [0] * 5
        patterns = [re.compile("\\bWINDSCRIBE_MAJOR_VERSION\\s+(\\d+)"),
                    re.compile("\\bWINDSCRIBE_MINOR_VERSION\\s+(\\d+)"),
                    re.compile("\\bWINDSCRIBE_BUILD_VERSION\\s+(\\d+)"),
                    re.compile("^#define\\s+WINDSCRIBE_IS_BETA"),
                    re.compile("^#define\\s+WINDSCRIBE_IS_GUINEA_PIG")]

        with open(version_file, "r") as f:
            for line in f:
                for i in range(len(patterns)):
                    matched = patterns[i].search(line)
                    if matched:
                        values[i] = int(matched.group(1)) if matched.lastindex > 0 else 1
                        break
        version_only = "{:d}.{:d}.{:d}".format(values[0], values[1], values[2])
        # version-only: x.y.z
        if not include_beta_suffix:
            return version_only
        else:
            if values[3]:
                return version_only + "_beta"
            elif values[4]:
                return version_only + "_guinea_pig"
            else:
                return version_only

    @staticmethod
    def __extract_mac_signing_params():
        version_file = os.path.join(pathhelper.COMMON_DIR, "utils", "executable_signature", "executable_signature_defs.h")
        pattern = re.compile("^#define\\s+MACOS_CERT_DEVELOPER_ID\\s+")
        key_name = ""
        with open(version_file, "r") as f:
            for line in f:
                matched = pattern.search(line)
                if matched:
                    key_name = line[len(matched.group(0)):].strip("\"\n")
                    break
        if len(key_name) == 0:
            raise IOError("The MACOS_CERT_DEVELOPER_ID define was not found in '{}'. "
                          "This entry is required for code signing and runtime signature verification."
                          .format(version_file))
        team_id = ""
        pattern = re.compile("\(([^]]+)\)")
        matched = pattern.search(key_name)
        if matched:
            team_id = matched.group(0).strip("()")
        if len(team_id) == 0:
            raise IOError("The Team ID could not be extracted from '{}'. "
                          "This entry is required for code signing and runtime signature verification.".format(key_name))
        developer_strings = (key_name, team_id)
        return developer_strings


# allows shell to directly call this module to get the version
if __name__ == "__main__":
    extractor = Extractor()
    # build_all.py is using True, so must use True here as well so .gitlab-ci.yml retrieves the proper version string.
    print(extractor.app_version(True))
