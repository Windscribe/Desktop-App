vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

set(SHA512_VALUE 244d637b8754ee142e37734b4b8c250cda5dd031781ab68cf9990b624ad8b9801be31a64f9233a0dd78afbb10f62093b3ec55d87727cb98f0d71b7f13ed57853)

set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)

if (VCPKG_TARGET_IS_OSX)
    set(VCPKG_TARGET_ARCHITECTURE_ORIGINAL "${VCPKG_TARGET_ARCHITECTURE}")
    set(VCPKG_OSX_ARCHITECTURES_ORIGINAL "${VCPKG_OSX_ARCHITECTURES}")

    # On MacOS we need to build twice and lipo the results together, since liboqs build system can't support multiple architectures in one build.
    if (VCPKG_OSX_ARCHITECTURES_ORIGINAL MATCHES "x86_64")
        set(VCPKG_TARGET_ARCHITECTURE "x64")
        set(VCPKG_OSX_ARCHITECTURES "x86_64")
        vcpkg_from_github(
            OUT_SOURCE_PATH SOURCE_PATH
            REPO open-quantum-safe/liboqs
            REF ${VERSION}
            SHA512 ${SHA512_VALUE}
            HEAD_REF main
            PATCHES 0000-macos-universal.patch
        )

        vcpkg_cmake_configure(
            SOURCE_PATH "${SOURCE_PATH}"
            OPTIONS
                -DCMAKE_INSTALL_PREFIX="${CURRENT_PACKAGES_DIR}"/x64
                -DOQS_BUILD_ONLY_LIB=ON
                -DOQS_DIST_BUILD=ON
                -DOQS_USE_OPENSSL=OFF
        )
        vcpkg_cmake_install()
        vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/liboqs")
    endif()

    if (VCPKG_OSX_ARCHITECTURES_ORIGINAL MATCHES "arm64")
        # Cleans source path
        set(VCPKG_TARGET_ARCHITECTURE "arm64")
        set(VCPKG_OSX_ARCHITECTURES "arm64")
        vcpkg_from_github(
            OUT_SOURCE_PATH SOURCE_PATH
            REPO open-quantum-safe/liboqs
            REF ${VERSION}
            SHA512 ${SHA512_VALUE}
            HEAD_REF main
            PATCHES 0000-macos-universal.patch
        )

        vcpkg_cmake_configure(
            SOURCE_PATH "${SOURCE_PATH}"
            OPTIONS
                -DCMAKE_INSTALL_PREFIX="${CURRENT_PACKAGES_DIR}"/arm64
                -DOQS_BUILD_ONLY_LIB=ON
                -DOQS_DIST_BUILD=ON
                -DOQS_USE_OPENSSL=OFF
        )
        vcpkg_cmake_install()
    endif()

    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib/pkgconfig")
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib/cmake")
    if (VCPKG_OSX_ARCHITECTURES_ORIGINAL MATCHES "arm64" AND VCPKG_OSX_ARCHITECTURES_ORIGINAL MATCHES "x86_64")
        vcpkg_execute_build_process(
            COMMAND "lipo" "${CURRENT_PACKAGES_DIR}/arm64/lib/liboqs.a" "${CURRENT_PACKAGES_DIR}/x64/lib/liboqs.a" -create -output liboqs.a
            WORKING_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib"
            LOGNAME "lipo_${TARGET_TRIPLET}.log"
        )
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/lib/pkgconfig/liboqs.pc" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/liboqs.pc")
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/lib/cmake/liboqs" "${CURRENT_PACKAGES_DIR}/lib/cmake/liboqs")
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/include" "${CURRENT_PACKAGES_DIR}/include")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/arm64")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/x64")
        vcpkg_fixup_pkgconfig()
    elseif (VCPKG_OSX_ARCHITECTURES_ORIGINAL MATCHES "arm64")
        file(RENAME "${CURRENT_PACKAGES_DIR}/arm64/lib/pkgconfig/liboqs.pc" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/liboqs.pc")
        file(RENAME "${CURRENT_PACKAGES_DIR}/arm64/lib/cmake/liboqs" "${CURRENT_PACKAGES_DIR}/lib/cmake/liboqs")
        file(RENAME "${CURRENT_PACKAGES_DIR}/arm64/include" "${CURRENT_PACKAGES_DIR}/include")
        file(RENAME "${CURRENT_PACKAGES_DIR}/arm64/lib/liboqs.a" "${CURRENT_PACKAGES_DIR}/lib/liboqs.a")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/arm64")
        vcpkg_fixup_pkgconfig()
    elseif (VCPKG_OSX_ARCHITECTURES_ORIGINAL MATCHES "x86_64")
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/lib/pkgconfig/liboqs.pc" "${CURRENT_PACKAGES_DIR}/lib/pkgconfig/liboqs.pc")
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/lib/cmake/liboqs" "${CURRENT_PACKAGES_DIR}/lib/cmake/liboqs")
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/include" "${CURRENT_PACKAGES_DIR}/include")
        file(RENAME "${CURRENT_PACKAGES_DIR}/x64/lib/liboqs.a" "${CURRENT_PACKAGES_DIR}/lib/liboqs.a")
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/x64")
        vcpkg_fixup_pkgconfig()
    endif()

    vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/liboqs")

    # reset variables
    set(VCPKG_TARGET_ARCHITECTURE ${VCPKG_TARGET_ARCHITECTURE_ORIGINAL})
    set(VCPKG_OSX_ARCHITECTURES ${VCPKG_OSX_ARCHITECTURES_ORIGINAL})

else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO open-quantum-safe/liboqs
        REF ${VERSION}
        SHA512 ${SHA512_VALUE}
        HEAD_REF main
    )

    vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
            -DBUILD_SHARED_LIBS=OFF
            -DOQS_BUILD_ONLY_LIB=ON
            -DOQS_DIST_BUILD=ON
            -DOQS_USE_OPENSSL=ON
            # This flag is required to build on Windows arm64 since it's not an officially supported platform
            -DOQS_PERMIT_UNSUPPORTED_ARCHITECTURE=ON
    )

    vcpkg_cmake_install()
    vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/liboqs")
    vcpkg_fixup_pkgconfig()
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(
    INSTALL "${SOURCE_PATH}/LICENSE.txt"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME copyright
)

