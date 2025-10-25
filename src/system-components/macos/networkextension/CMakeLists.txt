cmake_minimum_required(VERSION 3.15)
project("Windscribe System Extension")

enable_language(Swift)

find_package(c-ares REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Create the system extension target
add_executable(com.windscribe.client.networkextension MACOSX_BUNDLE
    main.mm
)

set_target_properties(com.windscribe.client.networkextension PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "systemextension"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.windscribe.client.networkextension"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.client.networkextension"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_ENABLE_BITCODE NO
    XCODE_ATTRIBUTE_SKIP_INSTALL NO
    XCODE_ATTRIBUTE_GCC_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12.0"
    XCODE_ATTRIBUTE_INSTALL_PATH "/Library/SystemExtensions"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/wireguard_extension/WireGuardNetworkExtension/WireGuardNetworkExtension-Bridging-Header.h"
    # This is necessary, otherwise XCode will not find the module.modulemap files
    XCODE_ATTRIBUTE_USER_HEADER_SEARCH_PATHS "\"${CMAKE_CURRENT_SOURCE_DIR}/wireguard_extension/WireGuardKitC\" \"${CMAKE_CURRENT_SOURCE_DIR}/wireguard_extension/WireGuardKitGo\""
)

# Link against required frameworks and libraries
target_link_libraries(com.windscribe.client.networkextension PRIVATE
    "-framework NetworkExtension"
    "-framework Foundation"
    "-framework Network"
    "-framework Security"
    "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    spdlog::spdlog
    c-ares::cares
)

add_subdirectory(splittunneling_extension)
add_subdirectory(wireguard_extension)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../data/provisioning_profile/networkextension.provisionprofile")
    add_custom_command(TARGET com.windscribe.client.networkextension POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../../../../data/provisioning_profile/networkextension.provisionprofile" $<TARGET_FILE_DIR:com.windscribe.client.networkextension>/../embedded.provisionprofile)
endif()