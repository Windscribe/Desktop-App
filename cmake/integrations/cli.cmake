# ------------------------------------------------------------------------------
# Integration Configuration: Windscribe CLI
# ------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/windscribe.cmake)

set(WS_SUPPORTED_PLATFORMS Linux)

set(FRONTEND_TYPE "cli")
set(INSTALLER_TYPE "cli")

list(APPEND WS_COMPILE_DEFINITIONS CLI_ONLY)
set(WS_SETTINGS_CLI "${WS_PRODUCT_NAME_LOWER}_cli")

list(APPEND WS_COMPILE_DEFINITIONS WS_SETTINGS_CLI="${WS_SETTINGS_CLI}")
set(WS_QT_COMPONENTS Network LinguistTools Test)

set(WS_LINUX_PACKAGE_NAME "windscribe-cli")
set(WS_LINUX_OUTPUT_NAME "windscribe-cli_@VERSION@_@SUFFIX@_@ARCH@")
