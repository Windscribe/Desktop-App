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
    REF fa20154d58ca609bf871a48542cc882b994f82bb
    SHA512 154dac7d8e92c4b9e2a58c9b3d31dae6c0a1909c74a91ae60bc03168a8261c7efa62b1321a9c529a25dee507280dce01115021a8b3ddf48928e7f27737220033
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
    if(VCPKG_TARGET_IS_OSX)
        set(ENV{LDFLAGS} "$ENV{LDFLAGS} -headerpad_max_install_names -L${CURRENT_HOST_INSTALLED_DIR}/lib")
    else()
        set(ENV{LDFLAGS} "$ENV{LDFLAGS}")
    endif()

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
