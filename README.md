# Windscribe 2.0 Desktop Application
This repo contains the complete source code for the Windscribe 2.0 app. This includes installer, service/helper, and GUI. 

## Windows
### Prerequisites

- Windows 10/11.
- Install git (https://git-scm.com/downloads). When installing Git, you can stick with all the default options presented to you by the installer.
- Clone the repository.
- Install Visual Studio Community Edition 2019 (run install_vs.bat from /common/prepare_build_environment/windows).
- Install Python 2.7.18 (run install_python.bat from common/prepare_build_environment/windows).
- Install Active Perl (install from https://www.activestate.com/products/perl/downloads/ or execute this code snippet).
    - powershell -Command "& $([scriptblock]::Create((New-Object Net.WebClient).DownloadString('https://platform.activestate.com/dl/cli/w20598w01/install.ps1'))) -activate-default ActiveState/Perl-5.32"
    - Alternately, you can install Strawberry Perl from https://strawberryperl.com/
- Install CMake v3.23.x (run install_cmake.bat from common/prepare_build_environment/windows).
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

### Install libraries

- Create a build-libs folder in the root of the cloned repository.

- Download the following dependency zips from https://nexus.int.windscribe.com/#browse/browse:client-desktop:client-desktop/dependencies/current/windows and extract them into the build-libs folder.
    - boost.zip
    - cares.zip
    - curl.zip
    - jom.zip
    - lzo.zip
    - openssl.zip
    - openvpn_2_5_4.zip
    - qt.zip
    - stunnel.zip
    - wireguard.zip
    - zlib.zip

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.  You can run 'build_all --sign --use-local-secrets' for a code-signed build, using the certificate from the 'Install signing certificate' section above, which will perform run-time signature verification checks on the executables.  Note that an unsigned build must be installed on your PC if you intend to debug the project using Qt Creator.

See `build_all --help` for other build options.

You will find the application logs in C:/Users/USER/AppData/Local/Windscribe/Windscribe2.

## Mac
### Prerequisites

- macOS Big Sur or macOS Monterey
- Install Xcode 13.2.1 (If on Monterey, you may use a newer version of Xcode 13, but 13.2.1 is the last version to support Big Sur)
    - Note: these downloads will require you to first login to your Apple account.
    - https://download.developer.apple.com/Developer_Tools/Xcode_13.2.1/Xcode_13.2.1.xip
- Install brew (brew.sh)
    - /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
- Install git (This step is optional, as git is bundled with Xcode)
    - brew install git
- Install Auto-Tools and 7-Zip:
    - brew install libtool
    - brew install automake
    - brew install p7zip
- The default install of Python 2 has been removed as of macOS 12.3. You'll need to install Python 2.7.18 from https://www.python.org/downloads/release/python-2718/ if you are using macOS >= 12.3.
- Install dmgbuild:
    - python -m pip install dmgbuild
- Install cmake v3.23.x from: https://cmake.org/download/
- Clone the repository.
- Install python deps:
    - python tools/bin/get-pip.py
    - python -m pip install -r tools/requirements.txt

### Install signing certificate (required)
- Install your Developer ID Application signing certificate from your Apple Developer account in Keychain Access.
- Edit common/utils/executable_signature/executable_signature_defs.h and set the MACOS_CERT_DEVELOPER_ID entry to match your Developer ID Application signing certificate.

### Install libraries

- Open a terminal in the root folder of the cloned repository.

- Create a build-libs folder.

- Download the following dependency zips using curl, as outlined in the example below. Downloading them from Nexus using your browser will cause GateKeeper to quarantine the executables (protoc, qmake, etc.) and refuse to run them.
  `curl --show-error --fail --cacert tools/cacert.pem --create-dirs -o build-libs/dependencyname.zip https://nexus.int.windscribe.com/repository/client-desktop/client-desktop/dependencies/current/macos/dependencyname.zip`
    - boost.zip
    - cares.zip
    - curl.zip
    - lzo.zip
    - openssl.zip
    - openvpn_2_5_4.zip
    - qt.zip
    - stunnel.zip
    - wireguard.zip

- Extract the dependency zips into the build-libs folder.

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.

See `build_all --help` for other build options.

You will find the application logs in ~/Library/Application Support/Windscribe/Windscribe2.

### Platform Notes:
    - If you make any changes to the helper source code (backend/mac/helper/src), you must increase the CFBundleVersion in backend/mac/helper/src/helper-info.plist. The installer only updates the helper if this bundle version number has changed.
    - The IKEv2 protocol will only function in builds produced by Windscribe.  It's implementation on macOS utilizes the NEVPNManager API, which requires the 'Personal VPN' entitlement (com.apple.developer.networking.vpn.api) and an embedded provisioning profile file.  If you wish to enable IKEv2 functionality, you will have to create an embedded provisioning file in your Apple Developer account and use it in the client project (See 'embedded.provisionprofile' in client/client.pro).

## Linux
### Prerequisites

Build process tested on Ubuntu 20.04/ZorinOS 16 (gcc 9.3.0).

-Install build requirements:
    - sudo apt-get install build-essential git curl patchelf ruby-dev rpm libpam0g-dev software-properties-common libgl1-mesa-dev
    - sudo apt-get update
    - sudo apt-get install wget autoconf libtool cmake fakeroot python
    - sudo gem i fpm -f

- Clone the repository.

- Install python deps:
    - python tools/bin/get-pip.py
    - python -m pip install -r tools/requirements.txt

### Install libraries

- Create a build-libs folder in the root of the cloned repository.

- Download the following dependency zips from https://nexus.int.windscribe.com/#browse/browse:client-desktop:client-desktop/dependencies/current/linux and extract them into the build-libs folder.
    - boost.zip
    - cares.zip
    - curl.zip
    - lzo.zip
    - openssl.zip
    - openvpn_2_5_4.zip
    - qt.zip
    - stunnel.zip
    - wireguard.zip

### Build the Windscribe 2.0 app

Go to subfolder tools and run 'build_all'. Assuming all goes well with the build, the installer will be placed in build-exe.

See `build_all --help` for other build options.

The application installs to /usr/local/windscribe.  You will find the application logs in ~/.local/share/Windscribe/Windscribe2.
