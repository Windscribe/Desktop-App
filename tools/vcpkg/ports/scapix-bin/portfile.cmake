set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

if(VCPKG_HOST_IS_OSX)
   execute_process(COMMAND "uname" "-m" OUTPUT_VARIABLE HOST_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
elseif(VCPKG_HOST_IS_WINDOWS)
   set(HOST_ARCH "AMD64")
elseif(VCPKG_HOST_IS_LINUX)
   set(HOST_ARCH "x86_64")
else()
   message(FATAL_ERROR "Unsupported platform.")
endif()

set(scapix_bin_hash_Darwin-arm64  49062f8dde6fac836b478b386c7d724be3f18ca8a84e4fe538b11a36d081772d34e7f9db7d218837f10e08a6a6f926e7ec7fe56a34c5bb58b473209e490fe94b)
set(scapix_bin_hash_Darwin-x86_64 3288a54ca6555223768241c3a731f53a770700c7cd2601067f209a0c14e3400c933359449ceeb679608eb2a8cba2c463b50e4559fc27767ff010c5a015df072a)
set(scapix_bin_hash_Linux-x86_64  b526df3bef8bf07a868a7ae2d6bf239599103abcc84551b681ddf6fe6c13bbab0d1b6abda6a910847aa0f143d4f802fd37c7c586952bbd1f273a9f2b154dab77)
set(scapix_bin_hash_Windows-AMD64 2047c7119022fee6236ba3d19113854ac7b0b42e3f663075f78fad27df9a1198c24740c92d89ae4840060e2d0341e31114fdf9509b8a51f0cf230287d24ab4fd)

vcpkg_download_distfile(ARCHIVE
   URLS "https://github.com/scapix-com/scapix-bin/archive/refs/tags/v${VERSION}-${CMAKE_HOST_SYSTEM_NAME}-${HOST_ARCH}.tar.gz"
   FILENAME "${PORT}_${VERSION}.tar.gz"
   SHA512 ${scapix_bin_hash_${CMAKE_HOST_SYSTEM_NAME}-${HOST_ARCH}}
)

vcpkg_extract_source_archive(SOURCE_PATH
   ARCHIVE "${ARCHIVE}"
   SOURCE_BASE "scapix_bin"
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/scapix-bin-config.cmake.in" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DSCAPIX_EXE_PATH=${CURRENT_PACKAGES_DIR}/tools/${PORT}/${CMAKE_HOST_SYSTEM_NAME}-${HOST_ARCH}
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_copy_tools(TOOL_NAMES scapix scapix_java
                 SEARCH_DIR ${SOURCE_PATH}/${CMAKE_HOST_SYSTEM_NAME}-${HOST_ARCH}
                 DESTINATION ${CURRENT_PACKAGES_DIR}/tools/${PORT}/${CMAKE_HOST_SYSTEM_NAME}-${HOST_ARCH}
                 AUTO_CLEAN)

file(INSTALL "${SOURCE_PATH}/lib" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")


vcpkg_download_distfile(LICENSE_PATH
    URLS "https://raw.githubusercontent.com/scapix-com/scapix/master/LICENSE.txt"
    FILENAME "SCAPIX_LICENSE"
    SKIP_SHA512
)

file(INSTALL "${LICENSE_PATH}" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)