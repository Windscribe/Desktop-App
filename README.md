# Windscribe 2.0 Desktop Application

This repo contains the complete source code for the Windscribe 2.0 app. This includes installer, service/helper, and GUI.

## Windows

### Prerequisites

- Windows 10/11.
- Install [git](https://git-scm.com/downloads). When installing Git, you can stick with all the default options presented to you by the installer.
- Clone the repository.
- Install Visual Studio Community Edition 2019 (run `install_vs.bat` from `/tools/prepare_build_environment/windows`).
- Install Python 3 via either the Microsoft Store or from [here](https://www.python.org/downloads/). Minimum tested version is 3.6.8.
- Install CMake v3.28.x or newer from [here](https://cmake.org/download/). The project will build with older versions of CMake, but you may encounter some warnings.
- Install Ninja v1.10.2 from [here](https://github.com/ninja-build/ninja/releases)
- Install vcpkg from [here](https://vcpkg.io/en/getting-started.html)
    - Create a `VCPKG_ROOT` environment variable referencing the full path to your vcpkg install folder.
    - Go to the vcpkg directory and `git checkout 576379156e82da642f8d1834220876759f13534d`.
    - Do the bootstrap step after the above command.
- Verify the following entries are in your System `PATH` environment variable. If they are not, add them to the System `PATH` environment variable and reboot.
    - `C:\Program Files\Git\cmd`
    - `C:\[folder containing ninja.exe]`
    - `C:\[folder containing cmake.exe]`
- Verify that `python3` is available in your System `PATH` environment variable.
  - If you installed Python from the Microsoft Store, enable the `python3.exe` execution alias in System Settings `Manage App Execution Aliases`.
  - If you installed Python from python.org, you can `mklink /path/to/your/python3.exe /path/to/your/python.exe`

### Install build script dependencies
- You will have to go to `Manage App Execution Aliases` in System Settings and disable app installer for `python.exe` and `python3.exe`

```python
  python3 -m pip install -r tools/requirements.txt
```

In case of `pyyaml` [building issue on Windows](https://github.com/yaml/pyyaml/issues/724#issuecomment-1638636728), execute the following:

```python
pip install "cython<3.0.0" && pip install --no-build-isolation pyyaml==6.0
pip install setuptools wheel
```

### Install signing certificate (optional)
- Copy your PFX code signing file to `installer/windows/signing/code_signing.pfx`.
- Edit (create) `tools/notarize.yml` and add the following line:
```
  windows-signing-cert-password: password-for-code-signing-pfx
```
- Edit `client/common/utils/executable_signature/executable_signature_defs.h` and set the `WINDOWS_CERT_SUBJECT_NAME` entry to match your certficate's name of signer field.

### Build libraries

Go to subfolder `tools/deps` and run the following scripts in order. Append `--arm64` to the command to build a library for Windows arm64. Libraries will be placed in `build-libs[-arm64]`.

```
install_qt
install_openvpn_dco
install_wintun
install_wireguard
install_wstunnel
```

### Build the Windscribe 2.0 app

Go to subfolder `tools` and run `build_all`. Assuming all goes well with the build, the installer will be placed in `build-exe`. You can run `build_all --sign --use-local-secrets` for a code-signed build, using the certificate from the [Install signing certificate](#install-signing-certificate-optional) section above, which will perform run-time signature verification checks on the executables. Note that an unsigned build must be installed on your PC if you intend to debug the project. Append `--arm64` to the command to build for Windows arm64.

See `build_all --help` for other build options.

### Logs

- Client app, location pings and installer helper: `C:/Users/<user_name>/AppData/Local/Windscribe/Windscribe2`
- Service: `<Windscribe_installation_path>/windscribeservice.log`
- Installer: `<Windscribe_installation_path>/log_installer.txt`
- Uninstaller: system Debug View
- `<Windscribe_installation_path>` defaults to `C:/Program Files/Windscribe`

## Mac

### Prerequisites

- macOS Big Sur or newer, but preferably at least Monterey.
- Install Xcode 14.2 (If on MacOS 11 Big Sur, you may use Xcode 13.2.1, but this is deprecated and not maintained going forward)
    - Xcode 15 is not currently supported. You may install a copy of Xcode 14.2 and use `sudo xcode-select --switch /path/to/your/xcode` to have both Xcode 13/14 and 15 simultaneously.
    - You must agree to the Xcode license (`sudo xcodebuild -license`)
    - You may need to run `xcodebuild -runFirstLaunch` or the Xcode GUI once if CMake complains that it can't find the compiler when building using the Xcode generator.
    - Note: these downloads will require you to first login to your Apple account.
      - https://developer.apple.com/services-account/download?path=/Developer_Tools/Xcode_14.2/Xcode_14.2.xip
- Install brew (brew.sh)
```bash
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
- Install git. This step is optional, as git is bundled with Xcode.
```bash
  brew install git
```
- Install Auto-Tools:
```bash
  brew install libtool
  brew install automake
```
- Install Python 3:
    - Minimum tested version is Python 3.6.8. 3.12.0 seems to have some issues with the python deps, so 3.11.6 is the latest recommended version. You may do this however you like, however `pyenv` is recommended:
```bash
  brew install pyenv
  echo 'if command -v pyenv 1>/dev/null 2>&1; then eval "$(pyenv init --path)"; fi"' >> ~/.zshrc
  pyenv install 3.11.6
  pyenv global 3.11.6
```
- Install dmgbuild:
```bash
  python3 -m pip install dmgbuild
```

- Install CMake v3.28.x or newer from [here](https://cmake.org/download/) and make sure that the cmake executable is in the path and available for execution. The project will build with older versions of CMake, but you may encounter some warnings.
- Install vcpkg from [here](https://vcpkg.io/en/getting-started.html)
    - Create a `VCPKG_ROOT` environment variable referencing the full path to your vcpkg install folder.
    - Go to the vcpkg directory and `git checkout 576379156e82da642f8d1834220876759f13534d`.
    - Do the bootstrap step after the above command.
- Clone the repository.
- Install python deps:
```python
  python3 -m pip install -r tools/requirements.txt
```
- Install golang (minimum version 1.18): follow instructions from `https://go.dev/doc/install`

### Install signing certificate (required)
- Install your Developer ID Application signing certificate from your Apple Developer account in Keychain Access.
- Edit `client/common/utils/executable_signature/executable_signature_defs.h` and set the `MACOS_CERT_DEVELOPER_ID` entry to match your Developer ID Application signing certificate.

### Build libraries

Go to subfolder `tools/deps` and run the following scripts in order. Libraries will be placed in `build-libs`.

```
install_qt
install_wireguard
install_wstunnel
```

### Build the Windscribe 2.0 app

Go to subfolder `tools` and run `build_all`. Assuming all goes well with the build, the installer will be placed in `build-exe`.

See `build_all --help` for other build options.

### Platform Notes:
- If you make any changes to the helper source code `backend/mac/helper/src`, you must increase the `CFBundleVersion` in `backend/mac/helper/src/helper-info.plist`. The installer only updates the helper if this bundle version number has changed.
- The IKEv2 protocol will only function in builds produced by Windscribe. Its implementation on macOS utilizes the NEVPNManager API, which requires the 'Personal VPN' entitlement (`com.apple.developer.networking.vpn.api`) and an embedded provisioning profile file. If you wish to enable IKEv2 functionality, you will have to create an embedded provisioning file in your Apple Developer account and use it in the client project (Search for `embedded.provisionprofile` in `client/CMakeLists.txt` for details on where to place the embedded provisioning profile).

### Logs

- Client app and location pings: `/Users/<user_name>/Library/Application Support/Windscribe/Windscribe2`
- Installer: `/Users/<user_name>/Library/Application Support/Windscribe/Windscribe/log_installer.txt`
- Helper: `/Library/Logs/com.windscribe.helper.macos/helper_log.txt`

## Linux

### Build with Docker

The repository contains Dockerfile to simplify building process. Skip all the other sections of this manual if you decide to use Docker.

- Generate builder image:
```bash
  sudo docker build -t ws-builder .
```
- Install vcpkg:
```bash
  sudo docker run --rm -v .:/w ws-builder /bin/bash -c "git clone https://github.com/Microsoft/vcpkg.git && git checkout 576379156e82da642f8d1834220876759f13534d && ./vcpkg/bootstrap-vcpkg.sh --disableMetrics"
```
- Build all the dependencies:
```bash
  for i in qt wireguard wstunnel; do sudo docker run --rm -v .:/w ws-builder /bin/bash -c "cd /w/tools/deps/ && ./install_$i"; done
```
- Build the application:
```bash
  sudo docker run --rm -v .:/w ws-builder /bin/bash -c "export VCPKG_ROOT=/w/vcpkg  && cd /w/tools/ && ./build_all"
```

### Prerequisites

Build process tested on Ubuntu 20.04/ZorinOS 16 (gcc 9.3.0).

- Install build requirements:
```bash
  sudo apt-get update
  sudo apt-get install build-essential git curl patchelf libpam0g-dev software-properties-common libgl1-mesa-dev fakeroot python3-pip zip unzip libnl-genl-3-dev pkg-config libcap-ng-dev wget autoconf libtool libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-util-dev libxcb-xinerama0-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev

  # install CMake 3.28.x or newer (default for Ubuntu 20.04 is 3.16.3).
  # Make sure that the cmake executable is in the path and available for execution.
  sudo snap install cmake --classic
```
- Install vcpkg from [here](https://vcpkg.io/en/getting-started.html)
    - Create a `VCPKG_ROOT` environment variable referencing the full path to your vcpkg install folder.
    - Go to the vcpkg directory and `git checkout 576379156e82da642f8d1834220876759f13534d`.
    - Do the bootstrap step after the above command.
- Install golang (minimum version 1.18): follow instructions from `https://go.dev/doc/install`
- Clone the repository.
- Install python deps:
```python
  python3 -m pip install -r tools/requirements.txt
```

### Build libraries

Go to subfolder `tools/deps` and run the following scripts in order. Libraries will be placed in `build-libs`. When building Qt6, you may encounter an error relating to `check_for_ulimit`. If so, downgrade to CMake version 3.24.

```
install_qt
install_wireguard
install_wstunnel
```

### Build the Windscribe 2.0 app

Go to subfolder `tools` and run `build_all`. Assuming all goes well with the build, the installer will be placed in `build-exe`.

See `build_all --help` for other build options.

The application installs to `/opt/windscribe`.

### Logs
- Client app and location pings: `~/.local/share/Windscribe/Windscribe2`
- Helper: `/opt/windscribe/helper_log.txt`

## Contributing

Please see our [Contributing Guidelines](https://github.com/Windscribe/Desktop-App/blob/master/CONTRIBUTING.md)

## License

The Windscribe Desktop Client app [License](https://github.com/Windscribe/Desktop-App/blob/master/LICENSE)
