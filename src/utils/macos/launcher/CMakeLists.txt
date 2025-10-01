cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(launcher_mac)

set(SOURCES
    appdelegate.h
    assets.xcassets
    base.lproj/Main.storyboard
    main.m
    appdelegate.mm
)

add_executable(WindscribeLauncher MACOSX_BUNDLE ${SOURCES})

target_link_libraries(WindscribeLauncher
                      PRIVATE
                      "-framework AppKit"
)

set_target_properties(WindscribeLauncher
                      PROPERTIES
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/info.plist
                      RESOURCE "base.lproj/Main.storyboard;installer/Assets.xcassets"
                      XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME appicon
                      XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17"
                      XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
                      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES"
                      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application"
                      XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
                      XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS windscribelauncher.entitlements
                      XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES YES
                      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
                      XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES"
                      XCODE_ATTRIBUTE_INFOPLIST_FILE info.plist
                      XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
                      XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12"
                      XCODE_ATTRIBUTE_PROVISIONING_PROFILE ""
                      XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER ""
                      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.launcher.macos"
                      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "launcher"
                      XCODE_ATTRIBUTE_SKIP_INSTALL "yes"
)
target_include_directories(WindscribeLauncher PRIVATE
                           ../../../client/common
)

