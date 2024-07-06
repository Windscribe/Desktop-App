cmake_minimum_required(VERSION 3.19)

project(scapix LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/scapix-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/scapix-config.cmake"
  INSTALL_DESTINATION "share/scapix"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/scapix-config.cmake"
  DESTINATION "share/scapix"
)
