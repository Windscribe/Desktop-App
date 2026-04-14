# ------------------------------------------------------------------------------
# Integration Configuration: Windscribe GUI
# ------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/windscribe.cmake)

set(WS_SUPPORTED_PLATFORMS Windows Darwin Linux)

set(FRONTEND_TYPE "gui")
set(INSTALLER_TYPE "gui")

set(WS_CLIENT_EXTRA_LIBS Qt6::Widgets)
set(WS_QT_COMPONENTS Widgets Network LinguistTools Test)
# For convenience, since many paths are relative to the installer directory
set(WS_INSTALLER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${INSTALLER_TYPE}")


###
### Windows
###

set(WS_WIN_APP_ICON "frontend/gui/resources/icons/win/windscribe.ico")

# User-visible IKEv2 connection name shown in Windows network settings.
set(WS_WIN_IKEV2_CONNECTION_NAME "Windscribe IKEv2")

set(WS_WIN_INSTALLER_TARGET "installer")
set(WS_WIN_BOOTSTRAP_TARGET "bootstrap")

# build-app dependencies
set(WS_WIN_APP_TARGETS
    ${WS_APP_TARGET} ${WS_CLI_TARGET} ${WS_APP_IDENTIFIER}Service InstallHelper WireguardService uninstall)
set(WS_WIN_PDB_TARGETS
    ${WS_APP_TARGET} ${WS_CLI_TARGET} ${WS_APP_IDENTIFIER}Service InstallHelper WireguardService)

if(WIN32)
    list(APPEND WS_ADDITIONAL_SUBDIRS
        src/windscribe-cli
        src/utils/windows/install_helper
        src/utils/windows/wireguard_service
        src/installer/${INSTALLER_TYPE}/windows/uninstaller)
endif()

# Windows additional files to copy into the installer package.
# Each entry is "source_path|dest_path". Use DIR: prefix for directory copies.
set(WS_WIN_ADDITIONAL_FILES
    "DIR:${WS_INSTALLER_DIR}/windows/additional_files/qt|qt"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/wintun/bin/wintun.dll|wintun.dll"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win10/ovpn-dco.cat|openvpndco/win10/ovpn-dco.cat"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win10/ovpn-dco.inf|openvpndco/win10/ovpn-dco.inf"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win10/ovpn-dco.sys|openvpndco/win10/ovpn-dco.sys"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win11/ovpn-dco.cat|openvpndco/win11/ovpn-dco.cat"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win11/ovpn-dco.inf|openvpndco/win11/ovpn-dco.inf"
    "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win11/ovpn-dco.sys|openvpndco/win11/ovpn-dco.sys"
    "${WS_INSTALLER_DIR}/windows/additional_files/driver_utils/${PACKAGE_ARCH}/devcon.exe|devcon.exe"
    "${WS_INSTALLER_DIR}/windows/additional_files/driver_utils/${PACKAGE_ARCH}/tapctl.exe|tapctl.exe"
    "${WS_INSTALLER_DIR}/windows/additional_files/splittunnel/${PACKAGE_ARCH}/windscribesplittunnel.cat|splittunnel/windscribesplittunnel.cat"
    "${WS_INSTALLER_DIR}/windows/additional_files/splittunnel/${PACKAGE_ARCH}/windscribesplittunnel.inf|splittunnel/windscribesplittunnel.inf"
    "${WS_INSTALLER_DIR}/windows/additional_files/splittunnel/${PACKAGE_ARCH}/windscribesplittunnel.sys|splittunnel/windscribesplittunnel.sys"
)

###
### macOS
###

set(WS_MAC_APP_ICON "frontend/gui/resources/icons/Mac/windscribe.icns")

# Bundle IDs for the various components of the macOS application.
set(WS_MAC_GUI_BUNDLE_ID "com.windscribe.client")
set(WS_MAC_HELPER_BUNDLE_ID "com.windscribe.helper.macos")
set(WS_MAC_LAUNCHER_BUNDLE_ID "com.windscribe.launcher.macos")
set(WS_MAC_INSTALLER_BUNDLE_ID "com.windscribe.installer.macos")
set(WS_MAC_SPLIT_TUNNEL_BUNDLE_ID "com.windscribe.client.splittunnelextension")

# This is the service name/label for the IKEv2 password in the keychain.
set(WS_MAC_IKEV2_KEYCHAIN_SERVICE "aaa.windscribe.com.password.ikev2")

# User-visible VPN description shown in macOS Network preferences for IKEv2.
set(WS_MAC_VPN_DESCRIPTION "Windscribe VPN")

# Name of the main .app bundle (e.g. "Windscribe.app").
set(WS_MAC_APP_BUNDLE_NAME "${WS_PRODUCT_NAME}.app")

# Full path to the installed .app bundle.
set(WS_MAC_APP_DIR "/Applications/${WS_MAC_APP_BUNDLE_NAME}")
# Arbitrary GID/UID for the application's macOS user and group.
set(WS_MAC_GID "518")
set(WS_MAC_UID "1639")

set(WS_MAC_LAUNCHER_TARGET "WindscribeLauncher")
set(WS_MAC_INSTALLER_TARGET "installer")

set(WS_MAC_INSTALLER_APP_NAME "${WS_PRODUCT_NAME}Installer")
set(WS_MAC_INSTALLER_BUNDLE_NAME "${WS_MAC_INSTALLER_APP_NAME}.app")

# macOS helpers that need openssl dylib paths fixed to @executable_path/../Frameworks/.
set(WS_MAC_RPATH_BINARIES openvpn)

# Qt frameworks to remove from macOS bundles after macdeployqt
set(WS_MAC_UNUSED_FRAMEWORKS QtQml QtQuick)

# macOS entitlements file for the main app executable (empty string to skip)
set(WS_MAC_ENTITLEMENTS "${CMAKE_BINARY_DIR}/src/client/engine.entitlements.configured")

# dmgbuild settings file and background image for macOS DMG creation.
set(WS_MAC_DMGBUILD_SETTINGS "${WS_INSTALLER_DIR}/macos/dmgbuild/dmgbuild_settings.py")
set(WS_MAC_DMGBUILD_BACKGROUND "${WS_INSTALLER_DIR}/macos/dmg/installer_background.png")

# build-app dependencies
set(WS_MAC_APP_TARGETS
    ${WS_APP_TARGET} ${WS_CLI_TARGET} ${WS_MAC_HELPER_BUNDLE_ID} ${WS_MAC_LAUNCHER_TARGET} ${WS_MAC_SPLIT_TUNNEL_BUNDLE_ID})

if(APPLE)
    list(APPEND WS_ADDITIONAL_SUBDIRS
        src/windscribe-cli
        src/utils/macos/launcher
        src/splittunneling/macos)
endif()

###
### Linux
###

set(WS_LINUX_PACKAGE_NAME "windscribe")
set(WS_LINUX_OUTPUT_NAME "windscribe_@VERSION@_@SUFFIX@_@ARCH@")
