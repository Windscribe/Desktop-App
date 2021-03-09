#Python script for extract version string from windscribe_version.h
# usage: python InputDir VersionFilePath

import re
import sys
import os
import fnmatch

def getMajorVersion(filepath):
    f = open(filepath, "r")
    pattern = re.compile("\\bWINDSCRIBE_MAJOR_VERSION\\s+(\\d+)")
    for line in f:
        finded_word = pattern.findall(line)
        for word in finded_word:
            return word

def getMinorVersion(filepath):
    f = open(filepath, "r")
    pattern = re.compile("\\bWINDSCRIBE_MINOR_VERSION\\s+(\\d+)")
    for line in f:
        finded_word = pattern.findall(line)
        for word in finded_word:
            return word

def getBuildVersion(filepath):
    f = open(filepath, "r")
    pattern = re.compile("\\bWINDSCRIBE_BUILD_VERSION\\s+(\\d+)")
    for line in f:
        finded_word = pattern.findall(line)
        for word in finded_word:
            return word

def isBetaVersion(filepath):
    f = open(filepath, "r")
    pattern = re.compile("^#define\\s+WINDSCRIBE_IS_BETA")
    for line in f:
        finded_word = pattern.findall(line)
        for word in finded_word:
            return True
    return False

def getVersionForDmg(versionStr):
    nums = versionStr.split('.')
    beta = ''
    if nums[2] == '0':
        beta = '_beta'
    return nums[0] + '_' + nums[1] + '_build' + nums[3] + beta


arguments = len(sys.argv) - 1
if arguments >= 1:
	major = getMajorVersion(sys.argv[1])
	minor = getMinorVersion(sys.argv[1])
	build = getBuildVersion(sys.argv[1])
	isBeta = isBetaVersion(sys.argv[1])
	strVersion = '{:d}_{:02d}_build{:d}'.format(int(major), int(minor), int(build))
	if isBeta == True:
		strVersion = strVersion + '_beta'
	print strVersion

else:
    print 'Incorrect input arguments'
