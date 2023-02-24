cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(launcher_mac)

set(SOURCES
    windscribelauncher/appdelegate.h
    windscribelauncher/assets.xcassets
    windscribelauncher/base.lproj/Main.storyboard
    windscribelauncher/main.m
    windscribelauncher/appdelegate.mm
)

add_executable(WindscribeLauncher MACOSX_BUNDLE ${SOURCES})

target_link_libraries(WindscribeLauncher
                      PRIVATE
                      "-framework AppKit"
)

set_target_properties(WindscribeLauncher
                      PROPERTIES
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/windscribelauncher/info.plist
                      RESOURCE "windscribelauncher/base.lproj/Main.storyboard;installer/Assets.xcassets"
)
target_include_directories(WindscribeLauncher PRIVATE
                           ../../../client/common
)

SET(CMAKE_XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME appicon)
SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++17")
SET(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
SET(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "YES")
SET(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Developer ID Application")
SET(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
SET(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS windscribelauncher/windscribelauncher.entitlements)
SET(CMAKE_XCODE_ATTRIBUTE_COMBINE_HIDPI_IMAGES YES)
SET(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM})
SET(CMAKE_XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES")
SET(CMAKE_XCODE_ATTRIBUTE_INFOPLIST_FILE windscribelauncher/info.plist)
SET(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")
SET(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.14")
SET(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE "")
SET(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
SET(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.launcher.macos")
SET(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "launcher")
SET(CMAKE_XCODE_ATTRIBUTE_SKIP_INSTALL "yes")

