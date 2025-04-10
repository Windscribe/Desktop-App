vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO open-quantum-safe/oqs-provider
    REF ${VERSION}
    SHA512 4f8056cb2fbc2a8684e2046b12a65820605b472df565cb814340e59e72cdf1d4abc6b915d92771160f3805433a9e40e722ca833495e5f3d753b56384490ec9f9
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
