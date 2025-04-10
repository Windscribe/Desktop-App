set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

# workaround for universal Mac OS architecture, otherwise configure error
list(LENGTH VCPKG_OSX_ARCHITECTURES osx_archs_num)
if(VCPKG_TARGET_IS_OSX)
    if(HOST_TRIPLET MATCHES "arm64*")
        set(VCPKG_OSX_ARCHITECTURES "arm64")
    else()
        set(VCPKG_OSX_ARCHITECTURES "x86_64")
    endif()
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO OpenVPN/openvpn
    REF 5662b3a8eb9e5744602a6da8cf3847455a0ba74f
    SHA512 785e0c19a0d2d45b412cf73283dda83c2e394846cbd010bde7e4f29c33abc53b111339959bf85e55fbef9ca564a7b35dd5769507414d657e30b8155a68d19c04
    PATCHES
        fix-cmakelist.patch
        anti-censorship.patch
        windows-static-openssl.patch
)

set(VCPKG_BUILD_TYPE release)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_configure_cmake(
      SOURCE_PATH "${SOURCE_PATH}"
      OPTIONS
        -DENABLE_PKCS11=OFF
        -DBUILD_TESTING=OFF
        -DPKG_CONFIG_EXECUTABLE="${CURRENT_HOST_INSTALLED_DIR}/tools/pkgconf/pkgconf"
    )
        vcpkg_install_cmake()
else()
    set(ENV{CFLAGS} "$ENV{CFLAGS} -I${CURRENT_HOST_INSTALLED_DIR}/include")
    set(ENV{CPPFLAGS} "$ENV{CPPFLAGS} -I${CURRENT_HOST_INSTALLED_DIR}/include")
    set(ENV{LDFLAGS} "$ENV{LDFLAGS} -L${CURRENT_HOST_INSTALLED_DIR}/lib")

    vcpkg_list(SET CONFIGURE_OPTIONS
        "--with-crypto-library=openssl"
        "OPENSSL_CFLAGS=-I${CURRENT_HOST_INSTALLED_DIR}/include"
        "OPENSSL_LIBS=-L${CURRENT_HOST_INSTALLED_DIR}/lib -lssl -lcrypto"
        "--disable-plugin-auth-pam"
        "--disable-plugin-down-root"
    )

    # Universal OSX architecture
    if(VCPKG_TARGET_IS_OSX AND osx_archs_num GREATER_EQUAL 2)
        vcpkg_list(APPEND CONFIGURE_OPTIONS "CFLAGS=-arch x86_64 -arch arm64 -mmacosx-version-min=10.14")
    endif()

    vcpkg_configure_make(
      SOURCE_PATH "${SOURCE_PATH}"
          AUTOCONFIG
      OPTIONS
        ${CONFIGURE_OPTIONS}
    )
        vcpkg_install_make(INSTALL_TARGET "install-exec")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_copy_tools(TOOL_NAMES openvpn AUTO_CLEAN)
endif()

file(
  INSTALL "${SOURCE_PATH}/COPYRIGHT.GPL"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)