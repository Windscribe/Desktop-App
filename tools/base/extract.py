#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: library for extraction functions

import os
import re
import pathhelper


class Extractor:
    # memoize so we don't have to read the file more than once per variable
    extracted_app_version = ""
    extracted_mac_signing_params = ("", "")

    def __init__(self):
        pass

    def app_version(self):
        if not Extractor.extracted_app_version:
            Extractor.extracted_app_version = self.__extract_app_version()
        return Extractor.extracted_app_version

    def mac_signing_params(self):
        if not Extractor.extracted_app_version:
            Extractor.extracted_mac_signing_params = self.__extract_mac_signing_params()
        return Extractor.extracted_mac_signing_params

    @staticmethod
    def __extract_app_version():
        version_file = os.path.join(pathhelper.COMMON_DIR, "version", "windscribe_version.h")
        values = [0] * 3
        patterns = [re.compile("\\bWINDSCRIBE_MAJOR_VERSION\\s+(\\d+)"),
                    re.compile("\\bWINDSCRIBE_MINOR_VERSION\\s+(\\d+)"),
                    re.compile("\\bWINDSCRIBE_BUILD_VERSION\\s+(\\d+)")]
        with open(version_file, "r") as f:
            for line in f:
                for i in range(len(patterns)):
                    matched = patterns[i].search(line)
                    if matched:
                        values[i] = int(matched.group(1)) if matched.lastindex > 0 else 1
                        break
        # version-only: x.y.z
        return "{:d}.{:d}.{:d}".format(values[0], values[1], values[2])

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
    print extractor.app_version()
