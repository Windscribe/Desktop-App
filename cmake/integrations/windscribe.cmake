# ------------------------------------------------------------------------------
# Common Windscribe Branding
# Included by gui.cmake and cli.cmake integrations.
# ------------------------------------------------------------------------------

set(WS_VERSION_MAJOR 2)
set(WS_VERSION_MINOR 22)
set(WS_VERSION_BUILD 4)
set(WS_BUILD_TYPE "guinea_pig")

# WS_APP_IDENTIFIER: Internal identifier, no spaces. Used for service names,
#   adapter names, pipe names, and other system identifiers.
set(WS_APP_IDENTIFIER "Windscribe")
# WS_PRODUCT_NAME: User-facing product name. May contain spaces.
#   Used in UI strings, registry keys, log messages, macOS .app bundle paths.
set(WS_PRODUCT_NAME "Windscribe")
# WS_PRODUCT_NAME_LOWER: Lowercase identifier, no spaces. Used for filesystem paths,
#   binary name prefixes, user/group names, and config directories.
set(WS_PRODUCT_NAME_LOWER "windscribe")
# WS_PRODUCT_NAME_UPPER: Uppercase identifier, no spaces. Used for all-caps identifiers.
set(WS_PRODUCT_NAME_UPPER "WINDSCRIBE")

# WS_APP_TARGET: CMake target name for the main application executable.
set(WS_APP_TARGET "Windscribe")
# WS_APP_EXECUTABLE_NAME: Name of the main application binary.
set(WS_APP_EXECUTABLE_NAME "Windscribe")
# WS_CLI_TARGET: CMake target name for the CLI executable. Empty if no CLI.
set(WS_CLI_TARGET "windscribe-cli")
# WS_CLI_EXECUTABLE_NAME: Name of the CLI binary.
set(WS_CLI_EXECUTABLE_NAME "windscribe-cli")
# WS_EXTRA_CONFIG_NAME: Filename for the advanced parameters config file.
set(WS_EXTRA_CONFIG_NAME "${WS_PRODUCT_NAME_LOWER}_extra.conf")

# WS_BUILD_LIBS_SUBDIR: Subdirectory of build-libs/ where dependency install scripts
#   place compiled artifacts.
set(WS_BUILD_LIBS_SUBDIR "windscribe")

set(WS_OUTPUT_PREFIX "Windscribe")

# Output filename templates for final packaged artifacts.
# Available placeholders: @VERSION@, @SUFFIX@, @ARCH@
# @SUFFIX@ resolves to the build type (e.g. "guinea_pig"), empty for stable.
# @ARCH@ resolves to the architecture (e.g. "amd64", "arm64").
set(WS_WIN_OUTPUT_NAME "${WS_OUTPUT_PREFIX}_@VERSION@_@SUFFIX@_@ARCH@")
set(WS_WIN_SYMBOLS_NAME "${WS_OUTPUT_PREFIX}Symbols_@VERSION@_@SUFFIX@_@ARCH@")
set(WS_MAC_OUTPUT_NAME "${WS_OUTPUT_PREFIX}_@VERSION@_@SUFFIX@")

# WS_COMPILE_DEFINITIONS: Extra preprocessor definitions to expose to C++ code.
#   Allows code in common components to add integration-specific behaviour.
set(WS_COMPILE_DEFINITIONS WS_IS_WINDSCRIBE)

# QSettings organization and application names.
set(WS_SETTINGS_ORG "Windscribe")
set(WS_SETTINGS_APP "Windscribe2")

# Branding information, primarily for Windows .rc files
set(WS_VENDOR "Windscribe Limited")
set(WS_VENDOR_URL "https://www.windscribe.com")
set(WS_SUPPORT_EMAIL "hello@windscribe.com")
set(WS_COPYRIGHT "Copyright (C) 2026 Windscribe Limited")

# Which helper binaries to bundle.
set(WS_BUNDLED_HELPER_NAMES openvpn ctrld wstunnel amneziawg)

# Shared libraries to bundle.
set(WS_SHARED_LIB_NAMES wsnet openssl)

# Common directories for macOS and Linux.
set(WS_POSIX_RUN_DIR "/var/run/windscribe")
set(WS_POSIX_CONFIG_DIR "/etc/windscribe")

###
### Windows
###

# This variable is here here instead of gui.cmake because it is used when templating dependencies from tools/deps.  It is not used in the cli integration.
# WS_WIN_CONFIG_SUBDIR: Subdirectory under Program Files used for config storage.
#   e.g. "Windscribe" -> "C:\Program Files\Windscribe\config"
set(WS_WIN_CONFIG_SUBDIR "${WS_PRODUCT_NAME}")

###
### Linux
###

set(WS_LINUX_INSTALL_DIR "/opt/windscribe")
set(WS_LINUX_TMP_DIR "/var/tmp/windscribe")
set(WS_LINUX_LOG_DIR "/var/log/windscribe")
set(WS_LINUX_USER_GROUP "windscribe")

set(WS_LINUX_APP_TARGETS
    ${WS_APP_TARGET} ${WS_CLI_TARGET} helper ${WS_PRODUCT_NAME_LOWER}-authhelper)

# Additional subdirectories to build (beyond client and helper which are always built).
if(UNIX AND NOT APPLE)
    set(WS_ADDITIONAL_SUBDIRS src/windscribe-cli src/utils/linux/authhelper)
endif()

# Binaries that need RPATH set to ${WS_LINUX_INSTALL_DIR}/lib.
# Paths are relative to ${WS_LINUX_INSTALL_DIR}.
set(WS_LINUX_RPATH_BINARIES
    ${WS_APP_EXECUTABLE_NAME}
    ${WS_CLI_EXECUTABLE_NAME}
    helper
    ${WS_PRODUCT_NAME_LOWER}-authhelper
    ${WS_PRODUCT_NAME_LOWER}openvpn
    lib/libssl.so.3
    lib/libcrypto.so.3
    lib/libwsnet.so
)
