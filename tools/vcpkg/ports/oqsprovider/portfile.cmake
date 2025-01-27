vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO open-quantum-safe/oqs-provider
    REF ${VERSION}
    SHA512 059f86466261bbf5cb75793c465e16de7c1b1d1c2cb08c9d2a0eb1b48a18c74de1d28c19f2a9d955d8faef83e3902690a0a8d72ff7dd45778ee6e8b82249198a
    HEAD_REF main
    PATCHES 0001-disable-tests.patch
)

vcpkg_replace_string("${SOURCE_PATH}/oqsprov/CMakeLists.txt" "\${OPENSSL_MODULES_PATH}" "\${CMAKE_INSTALL_PREFIX}/bin")
if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
            -DBUILD_TESTING=OFF
            -DOQS_PROVIDER_BUILD_STATIC=ON
    )
else()
    vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
    )
endif()

vcpkg_cmake_install()

set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

file(
    INSTALL "${SOURCE_PATH}/LICENSE.txt"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME copyright
)
