cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(installer_mac)

find_package(Boost REQUIRED COMPONENTS serialization)

find_library(COCOA_LUMBERJACK_LIBS
             NAMES CocoaLumberjack
             PATHS ${CMAKE_SOURCE_DIR}/Frameworks
             NO_DEFAULT_PATH)
find_package(Qt6 REQUIRED COMPONENTS
             Widgets
             Gui
             Svg
             LinguistTools)

qt_standard_project_setup(REQUIRES 6.5)

set(SOURCES
    Assets.xcassets
    helper/helper_mac.h
    helper/installhelper_mac.mm
    helper/installhelper_mac.h
    helper/helper_mac.mm
    installer/base_installer.h
    installer/base_installer.mm
    installer/downloader.h
    installer/downloader.mm
    installer/installer.h
    installer/installer.mm
    installer/installer_shim.mm
    installer/processes_helper.h
    installer/processes_helper.cpp
    Logger.h
    Logger.m
    LoggerHelper.h
    LoggerHelper.mm
    main.mm
)

set_source_files_properties(main.mm OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/resources/windscribe.tar.lzma)

qt_add_resources(rc ../../common/installer_mac.qrc)
qt_add_executable(installer MACOSX_BUNDLE ${SOURCES} ${rc})
add_subdirectory(../../common ${CMAKE_CURRENT_BINARY_DIR}/common)

set_target_properties(installer
                      PROPERTIES
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
                      RESOURCE "Assets.xcassets"
                      COMPILE_WARNING_AS_ERROR TRUE
)

target_include_directories(installer PRIVATE
                           ../../../client/common
                           ../../../common
                           ../../common
                           ./installer
)

target_link_directories(installer
                        PRIVATE
                        ${CMAKE_SOURCE_DIR}/Frameworks
)

target_link_libraries(installer
                      PRIVATE
                      Boost::serialization
                      "-framework AppKit"
                      "-framework Security"
                      "-framework ServiceManagement"
                      "${COCOA_LUMBERJACK_LIBS}"
                      Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
)

set_property(TARGET installer
             PROPERTY XCODE_EMBED_FRAMEWORKS
             "${CMAKE_SOURCE_DIR}/Frameworks/CocoaLumberjack.framework"
)

set_target_properties(installer PROPERTIES XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
set_target_properties(installer PROPERTIES XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY ON)

add_custom_command(TARGET installer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:installer>/../Resources)
add_custom_command(TARGET installer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/../../../build/backend/mac/helper/$<CONFIG>/ $<TARGET_FILE_DIR:installer>/../Library/LaunchServices)

set(CMAKE_XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
set(CMAKE_XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES YES)
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM})
set(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")
set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE "")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.installer.macos")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "Install Windscribe")

