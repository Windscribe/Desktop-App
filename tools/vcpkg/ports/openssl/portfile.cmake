if(EXISTS "${CURRENT_INSTALLED_DIR}/share/libressl/copyright"
    OR EXISTS "${CURRENT_INSTALLED_DIR}/share/boringssl/copyright")
    message(FATAL_ERROR "Can't build openssl if libressl/boringssl is installed. Please remove libressl/boringssl, and try install openssl again if you need it.")
endif()

if(NOT (VCPKG_TARGET_IS_IOS OR VCPKG_TARGET_IS_ANDROID OR VCPKG_TARGET_IS_WINDOWS))
    vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)
endif()

if (NOT "${VERSION}" MATCHES [[^([0-9]+)\.([0-9]+)\.([0-9]+)$]])
    message(FATAL_ERROR "Version regex did not match.")
endif()
set(OPENSSL_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(OPENSSL_VERSION_MINOR "${CMAKE_MATCH_2}")
set(OPENSSL_VERSION_FIX "${CMAKE_MATCH_3}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/vcpkg-cmake-wrapper.cmake" @ONLY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO sftcd/openssl
    REF e1ba81996c8d58ca73940a4f3205102bfb45c387
    SHA512 ed2cfaf245443dc828965979dd94d35d6b15e5281bdc9713cb1c4fa8f50a25c21ac696a4384c0aca37c3b1a7ba25940efbc6bd0438ddba6124d334cbecd0ddf5
    PATCHES
        apple-tvos.patch
        disable-install-docs.patch
        script-prefix.patch
        0002-super-large-padding-extension.patch
        0003-OpenSSL-anticen-Move-TLS-padding-before-the-domain-n.patch
        0004-Fix-compilation.patch
        0005-Use-random-unallocated-extension-ID-for-padding.patch
        windows/install-layout.patch
        windows/install-pdbs.patch
        unix/android-cc.patch
        unix/move-openssldir.patch
        unix/no-empty-dirs.patch
        unix/no-static-libs-for-shared.patch
        unix/osx-universal-arch.patch
)

vcpkg_list(SET CONFIGURE_OPTIONS
    enable-static-engine
    enable-capieng
    no-ssl3
    no-weak-ssl-ciphers
    no-tests
)

set(INSTALL_FIPS "")
if("fips" IN_LIST FEATURES)
    vcpkg_list(APPEND INSTALL_FIPS install_fips)
    vcpkg_list(APPEND CONFIGURE_OPTIONS enable-fips)
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    vcpkg_list(APPEND CONFIGURE_OPTIONS shared)
else()
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-shared no-module)
endif()

if(NOT "tools" IN_LIST FEATURES)
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-apps)
endif()

if(DEFINED OPENSSL_USE_NOPINSHARED)
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-pinshared)
endif()

if(NOT VCPKG_TARGET_IS_ANDROID AND NOT VCPKG_TARGET_IS_IOS)
    set(OPENSSL_NO_AUTOLOAD_CONFIG ON)
endif()
if(OPENSSL_NO_AUTOLOAD_CONFIG)
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-autoload-config)
endif()

if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    include("${CMAKE_CURRENT_LIST_DIR}/windows/portfile.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/install-pc-files.cmake")
else()
    include("${CMAKE_CURRENT_LIST_DIR}/unix/portfile.cmake")
endif()

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

