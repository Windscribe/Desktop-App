#!/usr/bin/env python
# ------------------------------------------------------------------------------
# Windscribe Build System
# Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
# ------------------------------------------------------------------------------
# Purpose: Removes secrets from repository in preparation for open-sourcing

import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(TOOLS_DIR)
COMMON_DIR = os.path.join(ROOT_DIR, "common")
sys.path.insert(0, TOOLS_DIR)

import base.messages as msg
import base.utils as utl
import deps.installutils as iutl


# re-creates the build_all.yml without the password_cert value
def removeWindowsSigningCertPassword():
  build_all_yml = os.path.join(TOOLS_DIR, "build_all.yml")

  # build new file contents
  new_file_lines = []
  with open(build_all_yml, "r") as f:
    for line in f:
      if "password_cert" in line:
        new_file_lines.append("  password_cert:\n")
      else:
        new_file_lines.append(line)
  new_file_contents = ''.join(new_file_lines)

  # overwrite the old file with new contents
  msg.Verbose("Updating " + build_all_yml)
  utl.CreateFileWithContents(build_all_yml, new_file_contents, True) 


# Replaces the hardcoded settings file with the open-source version 
def overwriteHardcodedSettingsWithOpenSourceVersion():
  # remove the secret-holding hardcoded settings file
  hardcoded_settings_file = os.path.join(COMMON_DIR, "utils", "hardcodedsettings.ini")
  if os.path.exists(hardcoded_settings_file):
    utl.RemoveFile(hardcoded_settings_file)

  # rename the open-source file to replace the deleted one
  hardcoded_settings_file_open_source = os.path.join(COMMON_DIR, "utils", "hardcodedsettings_opensource.ini")
  if not os.path.exists(hardcoded_settings_file_open_source):
    raise Exception("Open-source version of hardcoded settings does not exist: " + hardcoded_settings_file_open_source)
  utl.CopyFile(hardcoded_settings_file_open_source, hardcoded_settings_file)


# remove linux code-signing keys
def removeLinuxSecrets():
  linux_keys_dir = os.path.join(COMMON_DIR, "keys", "linux")
  
  linux_pubkey = os.path.join(linux_keys_dir, "key.pub")
  utl.RemoveFile(linux_pubkey)

  linux_privkey = os.path.join(linux_keys_dir, "key.pem")
  utl.RemoveFile(linux_privkey)

  linux_temp_pubkey = os.path.join(linux_keys_dir, "key_pub.txt")
  utl.RemoveFile(linux_temp_pubkey)


def removeMacSecrets():
  notarize_yml = os.path.join(TOOLS_DIR, "notarize.yml")
  utl.RemoveFile(notarize_yml)

  # notarize_kext.sh should be refactorable to use the notarize.yml passwords -- leave this here until then
  notarize_kext_script = os.path.join(ROOT_DIR, "backend", "mac", "kext", "notarize_kext.sh")
  utl.RemoveFile(notarize_kext_script)

  provisioning_profile = os.path.join(ROOT_DIR, "backend", "mac", "provisioning_profile")
  utl.RemoveDirectory(provisioning_profile)

  # we should be able to remove this from the internal repo all together -- leave here until that happens
  certificates_folder = os.path.join(COMMON_DIR, "prepare_build_environment", "mac", "certificates")
  utl.RemoveDirectory(certificates_folder)


# remove the cert
def removeWindowsSecrets():
  code_signing_cert = os.path.join(ROOT_DIR, "installer", "windows", "signing", "code_signing.pfx")
  utl.RemoveFile(code_signing_cert)
  removeWindowsSigningCertPassword()


# remove hooks and update the hardcoded settings
def removeGeneralSecrets():
  git_hooks = os.path.join(TOOLS_DIR, "hooks")
  utl.RemoveDirectory(git_hooks)
  overwriteHardcodedSettingsWithOpenSourceVersion()


if __name__ == "__main__":
  start_time = time.time()

  exitcode = 0
  try:
    removeMacSecrets()
    removeWindowsSecrets()
    removeLinuxSecrets()
    removeGeneralSecrets()
  except Exception as e:
    msg.Error(e)
    exitcode = 1

  # output elapsed time
  elapsed_time = time.time() - start_time
  if elapsed_time >= 60:
    msg.HeadPrint("All done: %i minutes %i seconds elapsed" % (elapsed_time / 60, elapsed_time % 60))
  else:
    msg.HeadPrint("All done: %i seconds elapsed" % elapsed_time)

  # done
  sys.exit(exitcode)

