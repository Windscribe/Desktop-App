# Windscribe 2.0 Desktop Application
This repo contains the complete source code for the Windscribe 2.0 app. This includes installer, service/helper, and GUI. 

## Windows
### Prerequisites

- Windows 10/11.
- Install git (https://git-scm.com/downloads). When installing Git, you can stick with all the default options presented to you by the installer.
- Clone the repository.
- Visual Studio Community Edition 2019 (run install_vs.bat from /common/prepare_build_environment/windows).
- Python 2.7.18 (run install_python.bat from common/prepare_build_environment/windows).
- Active Perl (install from https://www.activestate.com/products/perl/downloads/ or execute this code snippet).
    - powershell -Command "& $([scriptblock]::Create((New-Object Net.WebClient).DownloadString('https://platform.activestate.com/dl/cli/w20598w01/install.ps1'))) -activate-default ActiveState/Perl-5.32"
    - Alternately, you can install Strawberry Perl from https://strawberryperl.com/
- CMake v3.23.x (run install_cmake.bat from common/prepare_build_environment/windows).
- Verify the following entries are in your System PATH environment variable. If they are not, add them to the System PATH environment variable.  Reboot.
    - C:\Python27
    - C:\Python27\Scripts
    - C:\Perl64\site\bin (or equivalent Strawberry Perl site\bin folder)
    - C:\Perl64\bin (or equivalent Strawberry Perl bin folder)
    - C:\Program Files\Git\cmd

### Install build script dependencies
- You will have to go to 'Manage App Execution Aliases' in System Settings and disable app installer for python.exe and python3.exe
- python tools/bin/get-pip.py
- python -m pip install -r tools/requirements.txt

### Install signing certificate (optional)
- Copy your PFX code signing file to installer/windows/signing/code_signing.pfx.
- Edit (create) tools/notarize.yml and add the following line:
    - windows-signing-cert-password: password-for-code-signing-pfx
- Edit common/utils/executable_signature/executable_signature_defs.h and set the WINDOWS_CERT_SUBJECT_NAME entry to match your certficate's name of signer field.

### Build libraries

Go to subfolder tools/deps and run the following scripts in order. Libraries will be placed in build-libs.

- install_jom 
- install_openssl
- install_qt
- install_cares
- install_zlib
- install_curl
- install_boost
- install_lzo
- install_openvpn
- install_wireguard
- install_stunnel
- install_protobuf

#### Notes
- Some libraries depends on others. Jom is installed first and speeds up further builds. Almost all of the libraries depend on openssl. Openvpn depends on LZO. Curl depends on openssl and zlib.
- If you notice install or build scripts fail for seemingly no reason, try running each script from a fresh shell instance (CMD or gitbash). It appears to have something to do with a character limit on PATH or ENV variables.

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.  You can run 'build_all --sign --use-local-secrets' for a code-signed build, using the certificate from the 'Install signing certificate' section above, which will perform run-time signature verification checks on the executables.  Note that an unsigned build must be installed on your PC if you intend to debug the project using Qt Creator.

See `build_all --help` for other build options.

You will find the application logs in C:/Users/USER/AppData/Local/Windscribe/Windscribe2.

## Mac
### Prerequisites

- MacOS Big Sur or MacOS Monterey (We recommend building/developing only on a native machine. VM setups are not well tested)
- Install Xcode 13.2.1
    - Note: these downloads will require you to first login to your Apple account.
    - https://download.developer.apple.com/Developer_Tools/Xcode_13.2.1/Xcode_13.2.1.xip
- Install brew (brew.sh)
    - /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
- git (https://git-scm.com/downloads). This step is optional, as git is bundled with Xcode.
    - brew install git
- Clone the repository.
- Install Auto-Tools and 7-Zip:
    - brew install libtool
    - brew install automake
    - brew install p7zip
- Install Wireguard build tools (brew install go)
- Install dmgbuild:
    - python -m pip install dmgbuild
- Install cmake v3.23.x from: https://cmake.org/download/
- Install python deps:
    - python tools/bin/get-pip.py
    - python -m pip install -r tools/requirements.txt

### Install signing certificate (required)
- Install your Developer ID Application signing certificate from your Apple Developer account in Keychain Access.
- Edit common/utils/executable_signature/executable_signature_defs.h and set the MACOS_CERT_DEVELOPER_ID entry to match your Developer ID Application signing certificate.

### Build libraries

Go to subfolder tools/deps and run the following scripts in order. Libraries will be placed in build-libs.

- install_openssl
- install_qt
- install_cares
- install_boost
- install_curl
- install_lzo
- install_openvpn
- install_wireguard
- install_stunnel
- install_protobuf

### Notes on building libraries:
    - Some libraries depends on others. Almost all of the libraries depends on openssl. Openvpn depends on LZO.

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.

See `build_all --help` for other build options.

You will find the application logs in ~/Library/Application Support/Windscribe/Windscribe2.

### Platform Notes:
    - If you make any changes to the helper source code (backend/mac/helper/src), you must increase the CFBundleVersion in backend/mac/helper/src/helper-info.plist. The installer only updates the helper if this bundle version number has changed.
    - The IKEv2 protocol will only function in builds produced by Windscribe.  It's implementation on MacOS utilizes the NEVPNManager API, which requires the 'Personal VPN' entitlement (com.apple.developer.networking.vpn.api) and an embedded provisioning profile file.  If you wish to enable IKEv2 functionality, you will have to create an embedded provisioning file in your Apple Developer account and use it in the engine project (See 'embedded.provisionprofile' in backend/engine/engine.pro).

## Linux
### Prerequisites

Build process tested on Ubuntu 20.04/ZorinOS 16 (gcc 9.3.0).

- Install build tools:
  - sudo apt install build-essential software-properties-common libgl1-mesa-dev

- Install git:
  - sudo apt install git

- Install curl binary (for downloading archives):
  - sudo apt install curl

- Install patchelf (for fixing rpaths during packaging):
  - sudo apt install patchelf

- Install fpm (for making an .rpm package):
  - sudo apt-get install ruby-dev rpm && sudo gem i fpm -f

- Install libpam (required for building openvpn):
  - sudo apt-get install libpam0g-dev

- Install golang (required for building wireguard):
  - You need at least version 1.18.  If you have snap:
    - sudo snap install go --classic
  - Otherwise, follow instructions at: https://go.dev/doc/install.

- Install autoconf (required for building protobuf):
  - sudo apt-get install autoconf

- Install libtool (required for building protobuf):
  - sudo apt-get install libtool

- Install cmake (default build system generator for Qt 6):
  - sudo apt-get install cmake

- Install fakeroot:
  - sudo apt-get install fakeroot

- Install Qt platform plugin dependencies (https://doc.qt.io/qt-6/linux-requirements.html#platform-plugin-dependencies):
  - sudo apt-get install libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev

- Clone the repository.

- Install python2 (and build-system dependencies):
  - sudo apt install python2
  - NOTE: the following symlink will affect any pre-existing python-calling code that relies on python3
  - sudo ln -s /usr/bin/python2 /usr/bin/python
  - python tools/bin/get-pip.py
  - python -m pip install -r tools/requirements.txt

### Build libraries

Go to subfolder tools/deps and run the following scripts in order. Libraries will be placed in build-libs.

- install_openssl
- install_qt
- install_cares
- install_boost
- install_curl
- install_lzo
- install_openvpn
- install_wireguard
- install_stunnel
- install_protobuf

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.

See `build_all --help` for other build options.

The application installs to /usr/local/windscribe.  You will find the application logs in ~/.local/share/Windscribe/Windscribe2.
