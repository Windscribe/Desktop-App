cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(installer_mac)

find_package(Boost REQUIRED COMPONENTS serialization)
find_package(spdlog CONFIG REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS
             Widgets
             Gui
             Svg
             LinguistTools)

qt_standard_project_setup(REQUIRES 6.5)

set(SOURCES
    ../../../client/engine/engine/helper/ihelperbackend.h
    ../../../client/engine/engine/helper/helperbackend_mac.cpp
    ../../../client/engine/engine/helper/installhelper_mac.mm
    helper/helper_installer.cpp
    helper/helper_installer.h
    installer/base_installer.h
    installer/base_installer.mm
    installer/downloader.h
    installer/downloader.mm
    installer/installer.h
    installer/installer.mm
    installer/installer_shim.mm
    installer/processes_helper.h
    installer/processes_helper.cpp
    string_utils.h
    main.mm
)

set_source_files_properties(main.mm OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/resources/windscribe.tar.lzma)

qt_add_resources(rc ../../common/installer_mac.qrc)
qt_add_executable(installer MACOSX_BUNDLE ${SOURCES} ${rc})
add_subdirectory(../../common ${CMAKE_CURRENT_BINARY_DIR}/common)

set_target_properties(installer
                      PROPERTIES
                      OUTPUT_NAME "WindscribeInstaller"
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
                      COMPILE_WARNING_AS_ERROR TRUE
)

target_include_directories(installer PRIVATE
                           ../../../client/common
                           ../../../client/engine/engine/helper
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
                      spdlog::spdlog
                      "-framework AppKit"
                      "-framework Security"
                      "-framework ServiceManagement"
                      Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
)

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
set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "12.0")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE "")
set(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.windscribe.installer.macos")
set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "Install Windscribe")

install(TARGETS installer
   RUNTIME DESTINATION .
   LIBRARY DESTINATION "installer.app/Contents/Frameworks"
   BUNDLE DESTINATION .
)
