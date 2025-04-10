set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_download_distfile(ARCHIVE
   URLS "https://github.com/scapix-com/scapix/archive/refs/tags/v${VERSION}.tar.gz"
   FILENAME "scapix_${VERSION}.tar.gz"
   SHA512 89669a4c04c35e6c4f218917adf3d34fd6e45f541f2a2b2050d9dc7b6e7d96c7bb14c59d23b1db01dd5fcc8c30422a3c99f4c45c9134d91fecab09179d9da585
)
vcpkg_extract_source_archive(SOURCE_PATH
   ARCHIVE "${ARCHIVE}"
   SOURCE_BASE "scapix"
   PATCHES
      fix-project.patch
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}/config")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/scapix-config.cmake.in" DESTINATION "${SOURCE_PATH}/config")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/config"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup()

file(INSTALL "${SOURCE_PATH}/" DESTINATION "${CURRENT_PACKAGES_DIR}/src/${PORT}")

# install license
vcpkg_download_distfile(LICENSE_PATH
    URLS "https://raw.githubusercontent.com/scapix-com/scapix/master/LICENSE.txt"
    FILENAME "SCAPIX_LICENSE"
    SKIP_SHA512
)
file(INSTALL "${LICENSE_PATH}" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
