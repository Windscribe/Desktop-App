# ------------------------------------------------------------------------------
# Linux Packaging Configuration
# ------------------------------------------------------------------------------

# Common macro to copy Linux binaries and dependencies
macro(linux_copy_files DEST_DIR TARGET_NAME)
    # Copy binaries
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying binaries to package..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Windscribe>"
                "${DEST_DIR}/opt/windscribe/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:windscribe-cli>"
                "${DEST_DIR}/opt/windscribe/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:helper>"
                "${DEST_DIR}/opt/windscribe/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:windscribe-authhelper>"
                "${DEST_DIR}/opt/windscribe/"
    )

    # Copy shared libraries
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:wsnet>"
                "${DEST_DIR}/opt/windscribe/lib/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/libssl.so.3"
                "${DEST_DIR}/opt/windscribe/lib/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.so.3"
                "${DEST_DIR}/opt/windscribe/lib/"
    )

    # Copy custom dependencies
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/wireguard/windscribewireguard"
                "${DEST_DIR}/opt/windscribe/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/wstunnel/windscribewstunnel"
                "${DEST_DIR}/opt/windscribe/"
    )

    # Copy OpenVPN and ctrld binaries from vcpkg
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/openvpn/sbin/openvpn"
                "${DEST_DIR}/opt/windscribe/windscribeopenvpn"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/ctrld/ctrld"
                "${DEST_DIR}/opt/windscribe/windscribectrld"
    )

    # Copy open source licenses
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/licenses/open_source_licenses.txt"
                "${DEST_DIR}/opt/windscribe/"
    )

    # Fix RPATH for binaries and libraries
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(PATCHELF_EXECUTABLE)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Fixing RPATH for binaries..."
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/Windscribe"
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/windscribe-cli"
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/helper"
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/windscribe-authhelper"
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/windscribeopenvpn"
            COMMAND ${CMAKE_COMMAND} -E echo "Fixing RPATH for shared libs..."
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/lib/libssl.so.3"
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/lib/libcrypto.so.3"
            COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "/opt/windscribe/lib"
                    "${DEST_DIR}/opt/windscribe/lib/libwsnet.so"
        )
    endif()
endmacro()

set(PACKAGE_ARCH "amd64")
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(PACKAGE_ARCH "arm64")
endif()

set(PACKAGE_TYPE "gui")
set(PACKAGE_NAME "windscribe")
if(BUILD_CLI_ONLY)
    set(PACKAGE_TYPE "cli")
    set(PACKAGE_NAME "windscribe-cli")
endif()

set(INSTALLER_LINUX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/linux")

# DEB Packaging
if(BUILD_DEB)
    set(DEB_PACKAGE_DIR "${CMAKE_BINARY_DIR}/debian_package")
    set(DEB_PACKAGE_NAME "${PACKAGE_NAME}_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}_${PACKAGE_ARCH}")

    add_custom_target(package-deb
        DEPENDS build-app
    )

    # Create package directory structure
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DEB_PACKAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/opt/windscribe"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/opt/windscribe/lib"
    )

    # Copy DEBIAN control files and scripts
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/${PACKAGE_TYPE}/debian_package/DEBIAN"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN"
        COMMAND chmod +x "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/preinst"
        COMMAND chmod +x "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/postinst"
        COMMAND chmod +x "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/prerm"
    )

    # Update version in control file
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND sed -i "s/Version: .*/Version: ${PROJECT_VERSION}/"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/control"
    )

    # Copy common files (systemd services, scripts, polkit policy, etc.)
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/common"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}"
    )

    # Copy overlay files (package-specific systemd units, scripts)
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/${PACKAGE_TYPE}/overlay"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}"
    )

    linux_copy_files("${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}" package-deb)

    # Build DEB package
    find_program(DPKG_DEB_EXECUTABLE dpkg-deb)
    if(DPKG_DEB_EXECUTABLE)
        add_custom_command(TARGET package-deb POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Building DEB package..."
            COMMAND ${DPKG_DEB_EXECUTABLE} --build "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}.deb"
                    "${BUILD_EXE_DIR}/"
        )
    else()
        message(FATAL_ERROR "dpkg-deb not found - cannot build DEB package")
    endif()
endif()

# Create deploy target that depends on all enabled package types
add_custom_target(deploy ALL)
if(BUILD_DEB)
    add_dependencies(deploy package-deb)
endif()
if(BUILD_RPM)
    add_dependencies(deploy package-rpm-fedora)
endif()
if(BUILD_RPM_OPENSUSE)
    add_dependencies(deploy package-rpm-opensuse)
endif()

# RPM Architecture mapping
# RPM_BUILD_ARCH: architecture for rpmbuild (x86_64, aarch64)
# RPM_FILENAME_ARCH: architecture for final filename (x86_64, arm64)
set(RPM_BUILD_ARCH "${PACKAGE_ARCH}")
set(RPM_FILENAME_ARCH "${PACKAGE_ARCH}")
if(PACKAGE_ARCH STREQUAL "amd64")
    set(RPM_BUILD_ARCH "x86_64")
    set(RPM_FILENAME_ARCH "amd64")
elseif(PACKAGE_ARCH STREQUAL "arm64")
    set(RPM_BUILD_ARCH "aarch64")
    set(RPM_FILENAME_ARCH "arm64")
endif()

# Common macro to build RPM packages
macro(build_rpm_package TARGET_NAME RPM_TYPE_DIR DISTRO_SUFFIX DISTRO_NAME)
    set(RPM_BUILD_ROOT "$ENV{HOME}/rpmbuild")

    add_custom_target(${TARGET_NAME}
        DEPENDS build-app
    )

    # Setup RPM build environment
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/SPECS"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/BUILD"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/BUILDROOT"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/RPMS"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/SOURCES"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/BUILD/opt/windscribe"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/BUILD/opt/windscribe/lib"
    )

    # Copy spec file and update version
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${INSTALLER_LINUX_DIR}/${PACKAGE_TYPE}/${RPM_TYPE_DIR}/SPECS/windscribe_rpm.spec"
                "${RPM_BUILD_ROOT}/SPECS/"
        COMMAND sed -i "s/Version:.*/Version: ${PROJECT_VERSION}/"
                "${RPM_BUILD_ROOT}/SPECS/windscribe_rpm.spec"
    )

    # Copy files to BUILD directory
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/common"
                "${RPM_BUILD_ROOT}/BUILD"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/${PACKAGE_TYPE}/overlay"
                "${RPM_BUILD_ROOT}/BUILD"
    )

    # Copy binaries, libraries, and fix RPATH
    linux_copy_files("${RPM_BUILD_ROOT}/BUILD" ${TARGET_NAME})

    # Build RPM package
    find_program(RPMBUILD_EXECUTABLE rpmbuild)
    if(RPMBUILD_EXECUTABLE)
        set(RPM_SOURCE_NAME "${PACKAGE_NAME}-${PROJECT_VERSION}-0.${RPM_BUILD_ARCH}.rpm")
        set(RPM_FINAL_NAME "${PACKAGE_NAME}_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}_${RPM_FILENAME_ARCH}${DISTRO_SUFFIX}.rpm")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Building ${DISTRO_NAME} RPM package..."
            COMMAND ${RPMBUILD_EXECUTABLE} -bb "${RPM_BUILD_ROOT}/SPECS/windscribe_rpm.spec"
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${RPM_BUILD_ROOT}/RPMS/${RPM_BUILD_ARCH}/${RPM_SOURCE_NAME}"
                    "${BUILD_EXE_DIR}/${RPM_FINAL_NAME}"
        )
    else()
        message(FATAL_ERROR "rpmbuild not found - cannot build RPM package")
    endif()
endmacro()

# Fedora RPM Packaging
if(BUILD_RPM)
    build_rpm_package(package-rpm-fedora "rpm_fedora_package" "_fedora" "Fedora")
endif()

# OpenSUSE RPM Packaging
if(BUILD_RPM_OPENSUSE)
    build_rpm_package(package-rpm-opensuse "rpm_opensuse_package" "_opensuse" "OpenSUSE")
    # Both RPMs use the same staging directory, so they must build sequentially
    if(BUILD_RPM)
        add_dependencies(package-rpm-opensuse package-rpm-fedora)
    endif()
endif()
