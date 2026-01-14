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

set_source_files_properties(main.mm OBJECT_DEPENDS ${CMAKE_BINARY_DIR}/src/installer/mac/installer/installer.app/Contents/Resources/windscribe.tar.lzma)

qt_add_resources(rc ../../common/installer_mac.qrc)
qt_add_executable(installer MACOSX_BUNDLE ${SOURCES} ${rc})
add_subdirectory(../../common ${CMAKE_CURRENT_BINARY_DIR}/common)

add_dependencies(installer com.windscribe.helper.macos prep-installer-macos)

# Configure Info.plist with DEVELOPMENT_TEAM substitution
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist.configured
    @ONLY
)

set_target_properties(installer
                      PROPERTIES
                      OUTPUT_NAME "installer"
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist.configured
                      MACOSX_BUNDLE_EXECUTABLE_NAME "installer"
                      MACOSX_BUNDLE_BUNDLE_NAME "WindscribeInstaller"
                      MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
                      MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
                      MACOSX_BUNDLE_GUI_IDENTIFIER "com.windscribe.installer.macos"
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

target_compile_options(installer PRIVATE "-fobjc-arc")

target_link_libraries(installer
                      PRIVATE
                      Boost::serialization
                      spdlog::spdlog
                      "-framework AppKit"
                      "-framework Security"
                      "-framework ServiceManagement"
                      Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
)

# Copy installer's own resources (icon, etc.)
add_custom_command(TARGET installer POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:installer>/../Resources)

# Copy helper for installer to use during installation
add_custom_command(TARGET installer POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E make_directory
                   $<TARGET_FILE_DIR:installer>/../Library/LaunchServices)
add_custom_command(TARGET installer POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   $<TARGET_FILE:com.windscribe.helper.macos> $<TARGET_FILE_DIR:installer>/../Library/LaunchServices/)

install(TARGETS installer
   RUNTIME DESTINATION .
   LIBRARY DESTINATION "installer.app/Contents/Frameworks"
   BUNDLE DESTINATION .
)

# Rename bundle after installation
install(CODE "
    execute_process(COMMAND ${CMAKE_COMMAND} -E rename
        \${CMAKE_INSTALL_PREFIX}/installer.app
        \${CMAKE_INSTALL_PREFIX}/WindscribeInstaller.app)
")
