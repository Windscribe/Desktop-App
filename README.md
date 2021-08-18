# Windows
## Requirements before installation:

- Windows 10.
- Recommended 120+ GB of storage.
- Install git (https://git-scm.com/downloads). When installing Git, you can stick with all the default options presented to you by the installer.
- Clone the repository.
- Visual Studio Community 2017 (run install_vs.bat with admin rights from /common/prepare_build_environment/windows).
- Python 2.7x (run install_python.bat with admin rights from common/prepare_build_environment/windows).
- Active Perl (run install_activeperl.bat with admin rights from common/prepare_build_environment/windows).
- CMake (run install_cmake.bat from common/prepare_build_environment/windows).
- CppCheck (run install_cppcheck.bat with admin rights from common/prepare_build_environment/windows).
- The install_python.bat and install_perl.bat scripts may not update your System PATH environment variable. Reboot, then verify the following entries are in your System PATH environment variable. If they are not, add them to the System PATH environment variable and reboot again.
    - C:\Python27
    - C:\Python27\Scripts
    - C:\Perl64\site\bin
    - C:\Perl64\bin
    - C:\Program Files\Git\cmd
    - C:\Program Files\Cppcheck

## Install build script dependencies:
- python tools/bin/get-pip.py
- python -m pip install -r tools/requirements.txt

## Install signing certificate:
- copy your PFX-file installer/windows/signing/code_signing.pfx.

## Build libraries:

Go to subfolder tools/deps and run the following scripts in order. Libraries will be placed in client-desktop\build-libs.

1. install_jom 
2. install_openssl
3. install_qt
4. install_cares
5. install_zlib
6. install_curl
7. install_boost
8. install_lzo
9. install_openvpn
10. install_wireguard
11. install_stunnel
12. install_protobuf

Notes:
- Some libraries depends on others. Jom is installed first and speeds up further builds. Almost all of the libraries depends on openssl. Openvpn depends on LZO. Curl depends on openssl and zlib.
- If you notice install or build scripts fail for seemingly no reason, try running each script from a fresh shell instance (CMD or gitbash). It appears to have something to do with a character limit on PATH or ENV variables.

# Mac
## Build Environment Pre-Requisites

- MacOS Catalina or MacOS Big Sur (We recommend building/developing only on a native machine. VM setups are not well tested)
- Recommended 120 GB of storage (Need to take a closer look at this number, we may be able to get away with as little as 80GB)
- Install brew (brew.sh)
    - /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
- Xcode 11.3.1 (See below "Install Xcode") (MacOS Catalina) / 11.7 (Big Sur)
    - Note: these downloads will require successful AppleID login:
    - https://download.developer.apple.com/Developer_Tools/Xcode_11.7/Xcode_11.7.xip
    - https://download.developer.apple.com/Developer_Tools/Xcode_11.3.1/Xcode_11.3.1.xip

- git (https://git-scm.com/downloads)
    - brew install git

- Install your signing certificate in the Keychain
- Install Auto-Tools and 7-Zip (See below "Install Auto-Tools")
- Install Wireguard build tools (brew install swiftlint go)
- Install CppCheck (brew install cppcheck)
- Install dropDMG from: https://c-command.com/dropdmg/
    - Create symlink for dropdmg
        - ln -s /Applications/DropDMG.app/Contents/Frameworks/DropDMGFramework.framework/Versions/A/dropdmg /usr/local/bin/dropdmg
    - Disable auto-update
    - Enable "Quit when done"
    - Run once and accept the update command
    - Copy $REPO/common/prepare_build_environment/mac/DropDMG/Configurations and Layouts into ~HOME/Library/Application Support/DropDMG/Configurations and Layouts
    - See below for additional CI-runner setup
    - Install cmake (3.20.1 last tested) from: https://cmake.org/download/ 
    - Install python deps:
        - python tools/bin/get-pip.py
        - python -m pip install -r tools/requirements.txt

## Build Dependencies
1. Clone project repositories to local disk (See below: "Clone Desktop 2.0 repos").
2. Goto subfolder "client-desktop/tools/deps".
3. Run install_openssl
4. Run install_qt
5. Run install_cares
6. Run install_boost
7. Run install_curl
8. Run install_lzo
9. Run install_openvpn
10. Run install_wireguard
11. Run install_stunnel
12. Run install_protobuf
13. Run install_gtest

## Notes on building libraries:
    - Some libraries depends on others. Almost all of the libraries depends on openssl. Openvpn depends on LZO.
    - If you run the BASH-script without parameters, then it builds libraries and puts output binaries in ~/LibsWindscribe/xxx (for example, ~/LibsWindscribe/boost ~/LibsWindscribe/curl, etc...). Only Qt is placed in a separate folder ~/Qt. 
    
## Install Auto-Tools and 7-Zip (via HomeBrew):
    - /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
    - brew install libtool
    - brew install automake
    - brew install p7zip

## Notes on building Windscribe
To disable executable signature checks in helper, please run the build script with "private" argument. E.g.:
    ./build_all.sh private


