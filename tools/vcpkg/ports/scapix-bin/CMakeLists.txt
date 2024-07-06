cmake_minimum_required(VERSION 3.19)

project(scapix-bin LANGUAGES C CXX)

set(SCAPIX_EXE ${SCAPIX_EXE_PATH} CACHE PATH "Location of scapix executables")


include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scapix-bin-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/scapix-bin-config.cmake"
  INSTALL_DESTINATION "share/scapix-bin"
  PATH_VARS SCAPIX_EXE
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/scapix-bin-config.cmake"
  DESTINATION "share/scapix-bin"
)
