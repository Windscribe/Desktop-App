cmake_minimum_required(VERSION 3.15)
project(com.windscribe.client.splittunnelextension)

find_package(c-ares REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Create the system extension target
add_executable(com.windscribe.client.splittunnelextension MACOSX_BUNDLE
    FlowTCP.mm
    FlowUDP.mm
    ip_hostnames/ares_library_init.cpp
    ip_hostnames/dns_resolver.cpp
    ip_hostnames/ip_hostnames_manager.cpp
    main.mm
    Settings.mm
    TransparentProxyProvider.mm
    Utils.mm
)

set_target_properties(com.windscribe.client.splittunnelextension PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "systemextension"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.windscribe.client.splittunnelextension"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.client.splittunnelextension"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
    XCODE_ATTRIBUTE_ENABLE_BITCODE NO
    XCODE_ATTRIBUTE_SKIP_INSTALL NO
    XCODE_ATTRIBUTE_GCC_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
    XCODE_ATTRIBUTE_INSTALL_PATH "/Library/SystemExtensions"
)

# Link against required frameworks and libraries
target_link_libraries(com.windscribe.client.splittunnelextension PRIVATE
    "-framework NetworkExtension"
    "-framework Foundation"
    "-framework Network"
    "-framework Security"
    "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    spdlog::spdlog
    c-ares::cares
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../provisioning_profile/splittunnelextension.provisionprofile")
    add_custom_command(TARGET com.windscribe.client.splittunnelextension POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/../provisioning_profile/splittunnelextension.provisionprofile" $<TARGET_FILE_DIR:com.windscribe.client.splittunnelextension>/../embedded.provisionprofile)
endif()

# Install rules
install(TARGETS com.windscribe.client.splittunnelextension
    BUNDLE DESTINATION .
    COMPONENT Runtime
)
