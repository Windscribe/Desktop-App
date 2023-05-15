cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(installer_mac)

set(SOURCES
    QuietModeHelper.h
    Assets.xcassets
    AppDelegate.h
    ToggleControl.h
    ForegroundView.h
    ForegroundView.m
    MainWindow.h
    LoggerHelper.h
    LoggerHelper.mm
    ProgressView.h
    ImageResources.mm
    main.mm
    ImageResources.h
    ProgressView.mm
    AppDelegate.mm
    MainView.mm
    ToggleControl.mm
    MainView.h
    MainWindow.mm
    QuietModeHelper.mm
    Logger.h
    Logger.m
    Base.lproj/MainMenu.xib
    Buttons/ButtonsCells.h
    Buttons/ButtonsCells.m
    Buttons/HoverButton.h
    Buttons/HoverButton.m
    installer/base_installer.hpp
    installer/downloader.hpp
    installer/installer.hpp
    installer/installer.mm
    installer/downloader.mm
    installer/processes_helper.h
    installer/processes_helper.cpp
    installer/base_installer.mm
    helper/helper_mac.h
    helper/installhelper_mac.mm
    helper/installhelper_mac.h
    helper/helper_mac.cpp
)

add_executable(installer MACOSX_BUNDLE ${SOURCES})

set_target_properties(installer
                      PROPERTIES
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
                      RESOURCE "Base.lproj/MainMenu.xib;Assets.xcassets"
)

target_include_directories(installer PUBLIC
                           Frameworks/SVGKit.framework/Versions/A/Headers
)

find_library(COCOA_LUMBERJACK_LIBS
             NAMES CocoaLumberjack
             PATHS ${CMAKE_SOURCE_DIR}/Frameworks
             NO_DEFAULT_PATH)
find_library(SVGKIT_LIBS
             NAMES SVGKit
             PATHS ${CMAKE_SOURCE_DIR}/Frameworks
             NO_DEFAULT_PATH)

target_include_directories(installer PRIVATE
                           ../../../build-libs/boost/include
                           ../../../client/common
)

target_link_directories(installer
                        PRIVATE
                        ../../../build-libs/boost/lib
                        ${CMAKE_SOURCE_DIR}/Frameworks
)

target_link_libraries(installer
                      PRIVATE
                      libboost_serialization.a
                      "-framework AppKit"
                      "-framework QuartzCore"
                      "-framework Security"
                      "-framework ServiceManagement"
                      "${COCOA_LUMBERJACK_LIBS}"
                      "${SVGKIT_LIBS}"
)

set_property(TARGET installer
             PROPERTY XCODE_EMBED_FRAMEWORKS
             "${CMAKE_SOURCE_DIR}/Frameworks/CocoaLumberjack.framework"
             "${CMAKE_SOURCE_DIR}/Frameworks/SVGKit.framework"
)
set_target_properties(installer PROPERTIES XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
set_target_properties(installer PROPERTIES XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY ON)

add_custom_command(TARGET installer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:installer>/../Resources)
add_custom_command(TARGET installer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/svg $<TARGET_FILE_DIR:installer>/../Resources)
add_custom_command(TARGET installer POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:installer>/../Resources)
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
set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.14")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE "")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.installer.macos")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "installer")

