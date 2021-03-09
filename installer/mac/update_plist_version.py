#Python script for update CFBundleVersion in all plist files in input directory
# usage 1: python UpdatePlistVersion.py InputDir VersionFilePath
# usage 2, return version as string: python UpdatePlistVersion.py VersionFilePath
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


def modifyVersionInPlist(plistFilePath, versionStr):
    with open(plistFilePath, 'r') as file:
        filedata = file.read()

    filedata = re.sub('<key>CFBundleVersion</key>\n[^\n]+', '<key>CFBundleVersion</key>\n\t<string>' + versionStr + '</string>', filedata, flags = re.M)

    with open(plistFilePath, 'w') as file:
        file.write(filedata)

#strVersion = getVersionString('../windscribe.pro').strip()
#modifyVersionInPlist('Info.plist', strVersion)
#print getVersionForDmg(strVersion)
arguments = len(sys.argv) - 1
if arguments >= 2:
    major = getMajorVersion(sys.argv[2])
    minor = getMinorVersion(sys.argv[2])
    build = getBuildVersion(sys.argv[2])
    isBeta = isBetaVersion(sys.argv[2])
    print 'Modify %s with version: %s.%s.%s' % (sys.argv[1], major, minor, build)
    
    strVersion = '{0}.{1}.{2}'.format(major, minor, build)
    modifyVersionInPlist(sys.argv[1], strVersion)

elif arguments == 1:
    major = getMajorVersion(sys.argv[1])
    minor = getMinorVersion(sys.argv[1])
    build = getBuildVersion(sys.argv[1])
    isBeta = isBetaVersion(sys.argv[1])
    strVersion = '{:d}_{:02d}_build{:d}'.format(int(major), int(minor), int(build))
    if isBeta == True:
        strVersion = strVersion + '_beta'
    print strVersion
else:
    print 'Incorrect arguments'
