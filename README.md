# Windscribe 2.0 Desktop Application
This repo contains the complete source code for the Windscribe 2.0 app. This includes installer, service/helper, backend process and GUI. 
Please note that this is a work-in-progress, your mileage may vary.

## Windows
### Prerequisites

- Windows 10.
- Recommended 120+ GB of storage.
- Install git (https://git-scm.com/downloads). When installing Git, you can stick with all the default options presented to you by the installer.
- Clone the repository.
- Visual Studio Community 2017 (run install_vs.bat with admin rights from /common/prepare_build_environment/windows).
- Python 2.7x (run install_python.bat with admin rights from common/prepare_build_environment/windows).
- Active Perl (install from https://www.activestate.com/products/perl/downloads/ or execute this code snippet).
    - powershell -Command "& $([scriptblock]::Create((New-Object Net.WebClient).DownloadString('https://platform.activestate.com/dl/cli/w20598w01/install.ps1'))) -activate-default ActiveState/Perl-5.32"
    - Alternately, you can install Strawberry Perl from https://strawberryperl.com/
- CMake (run install_cmake.bat from common/prepare_build_environment/windows).
- CppCheck (run install_cppcheck.bat with admin rights from common/prepare_build_environment/windows).
- Verify the following entries are in your System PATH environment variable. If they are not, add them to the System PATH environment variable.  Reboot.
    - C:\Python27
    - C:\Python27\Scripts
    - C:\Perl64\site\bin (or equivalent Strawberry Perl site\bin folder)
    - C:\Perl64\bin (or equivalent Strawberry Perl bin folder)
    - C:\Program Files\Git\cmd
    - C:\Program Files\Cppcheck

### Install build script dependencies
- On Windows 10, you will have to go to 'Manage App Execution Aliases' in System Settings and disable app installer for python.exe and python3.exe
- python tools/bin/get-pip.py
- python -m pip install -r tools/requirements.txt

### Install signing certificate
- Copy your PFX code signing file to installer/windows/signing/code_signing.pfx.
- Edit tools/build_all.yml and enter the password for your PFX file in the password_cert field of the windows_signing_cert section.
- Note that the application will still build and run without these signatures, but using the VPN without signatures will require a debug build.

### Build libraries

Go to subfolder tools/deps and run the following scripts in order. Libraries will be placed in build-libs.

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

#### Notes
- Some libraries depends on others. Jom is installed first and speeds up further builds. Almost all of the libraries depends on openssl. Openvpn depends on LZO. Curl depends on openssl and zlib.
- If you notice install or build scripts fail for seemingly no reason, try running each script from a fresh shell instance (CMD or gitbash). It appears to have something to do with a character limit on PATH or ENV variables.

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.  You can run 'build_all debug' for a debug build. 
Note that a debug build is required to connect the VPN when building without code signing.

## Mac
### Prerequisites

- MacOS Catalina or MacOS Big Sur (We recommend building/developing only on a native machine. VM setups are not well tested)
- Recommended 120 GB of storage (Need to take a closer look at this number, we may be able to get away with as little as 80GB)
- Install brew (brew.sh)
    - /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
- Install Xcode 11.3.1 if using MacOS Catalina, or Xcode 11.7 if using Big Sur
    - Note: these downloads will require successful AppleID login:
    - https://download.developer.apple.com/Developer_Tools/Xcode_11.7/Xcode_11.7.xip
    - https://download.developer.apple.com/Developer_Tools/Xcode_11.3.1/Xcode_11.3.1.xip
    - The brew install step above will have installed the Xcode command-line tools.  Make sure to run Xcode after installing it, and set the Command Line Tools in Preferences->Locations to the version of Xcode you installed.

- git (https://git-scm.com/downloads). This step is optional, as git is bundled with Xcode.
    - brew install git
- Clone the repository.
- Install Auto-Tools and 7-Zip (See below "Install Auto-Tools")
- Install Wireguard build tools (brew install go)
- Install CppCheck (brew install cppcheck)
- Install dropDMG from: https://c-command.com/dropdmg/
    - Create symlink for dropdmg
        - ln -s /Applications/DropDMG.app/Contents/Frameworks/DropDMGFramework.framework/Versions/A/dropdmg /usr/local/bin/dropdmg
    - Run the DropDMG app and accept the update command-line tools prompt
    - Disable auto-update
    - Enable "Quit when done" in Preferences->Advanced
    - Copy desktop-v2/common/prepare_build_environment/mac/DropDMG/Configurations and Layouts into ~HOME/Library/Application Support/DropDMG/Configurations and Layouts
- Install cmake (3.20.1 last tested) from: https://cmake.org/download/ 
- Install python deps:
  - python tools/bin/get-pip.py
  - python -m pip install -r tools/requirements.txt

### Set up code signing for release builds
- Install your Developer ID signing certificate in the Keychain.
- TBD...

### Build Dependencies
- Open a terminal in desktop-v2/tools/deps:
- Run install_openssl
- Run install_qt
- Run install_cares
- Run install_boost
- Run install_curl
- Run install_lzo
- Run install_openvpn
- Run install_wireguard
- Run install_stunnel
- Run install_protobuf
- Run install_gtest

### Notes on building libraries:
    - Some libraries depends on others. Almost all of the libraries depends on openssl. Openvpn depends on LZO.
    - The install scripts put the dependencies in desktop-v2/build-libs.

### Build the Windscribe 2.0 app

Open a terminal in desktop-v2/tools and run './build_all' for a release build, or './build_all debug' for a debug build.  Please note that a release build requires you to have completed the 'Set up code signing' section above.  Assuming all goes well with the build, the installer will be placed in desktop-v2/build-exe.

### Install Auto-Tools and 7-Zip (via HomeBrew):
    - brew install libtool
    - brew install automake
    - brew install p7zip
