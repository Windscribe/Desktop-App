# ------------------------------------------------------------------------------
# Linux Packaging Configuration
# ------------------------------------------------------------------------------

# Common macro to copy Linux binaries and dependencies
macro(linux_copy_files DEST_DIR TARGET_NAME)
    # Copy app target binaries
    foreach(_target ${WS_LINUX_APP_TARGETS})
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${_target}>" "${DEST_DIR}${WS_LINUX_INSTALL_DIR}/"
        )
    endforeach()

    # Copy shared libraries
    foreach(_entry ${WS_SHARED_LIBS})
        string(REPLACE "|" ";" _pair "${_entry}")
        list(GET _pair 0 _src)
        list(GET _pair 1 _dest)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "${DEST_DIR}${WS_LINUX_INSTALL_DIR}/lib/${_dest}"
        )
    endforeach()

    # Copy bundled helper binaries
    foreach(_entry ${WS_BUNDLED_HELPERS})
        string(REPLACE "|" ";" _pair "${_entry}")
        list(GET _pair 0 _src)
        list(GET _pair 1 _dest)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "${DEST_DIR}${WS_LINUX_INSTALL_DIR}/${_dest}"
        )
    endforeach()

    # Copy open source licenses
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/client/client-common/licenses/open_source_licenses.txt"
                "${DEST_DIR}${WS_LINUX_INSTALL_DIR}/"
    )

    # Fix RPATH for binaries and libraries
    find_program(PATCHELF_EXECUTABLE patchelf)
    if(PATCHELF_EXECUTABLE)
        foreach(_bin ${WS_LINUX_RPATH_BINARIES})
            add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${PATCHELF_EXECUTABLE} --set-rpath "${WS_LINUX_INSTALL_DIR}/lib"
                        "${DEST_DIR}${WS_LINUX_INSTALL_DIR}/${_bin}"
            )
        endforeach()
    endif()
endmacro()

set(INSTALLER_LINUX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${INSTALLER_TYPE}/linux")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${INSTALLER_TYPE}/linux/common")
    set(INSTALLER_LINUX_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${INSTALLER_TYPE}/linux/common")
else()
    set(INSTALLER_LINUX_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${WS_PRODUCT_NAME_LOWER}/linux")
endif()

# DEB Packaging
if(BUILD_DEB)
    set(DEB_PACKAGE_DIR "${CMAKE_BINARY_DIR}/debian_package")
    ws_resolve_output_name("${WS_LINUX_OUTPUT_NAME}" DEB_PACKAGE_NAME)

    add_custom_target(package-deb
        DEPENDS build-app
    )

    # Create package directory structure
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${DEB_PACKAGE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}${WS_LINUX_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}${WS_LINUX_INSTALL_DIR}/lib"
    )

    # Copy DEBIAN control files and scripts
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/debian_package/DEBIAN"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN"
        COMMAND chmod +x "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/preinst"
        COMMAND chmod +x "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/postinst"
        COMMAND chmod +x "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/prerm"
    )

    # Update version and architecture in control file
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND sed -i "s/Version: .*/Version: ${PROJECT_VERSION}/"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/control"
        COMMAND sed -i "s/Architecture: .*/Architecture: ${PACKAGE_ARCH}/"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}/DEBIAN/control"
    )

    # Copy common files (systemd services, scripts, polkit policy, etc.)
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_COMMON_DIR}"
                "${DEB_PACKAGE_DIR}/${DEB_PACKAGE_NAME}"
    )

    # Copy overlay files (package-specific systemd units, scripts)
    add_custom_command(TARGET package-deb PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/overlay"
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
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/BUILD${WS_LINUX_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RPM_BUILD_ROOT}/BUILD${WS_LINUX_INSTALL_DIR}/lib"
    )

    # Copy spec file and update version
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${INSTALLER_LINUX_DIR}/${RPM_TYPE_DIR}/SPECS/rpm.spec"
                "${RPM_BUILD_ROOT}/SPECS/"
        COMMAND sed -i "s/Version:.*/Version: ${PROJECT_VERSION}/"
                "${RPM_BUILD_ROOT}/SPECS/rpm.spec"
    )

    # Copy files to BUILD directory
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_COMMON_DIR}"
                "${RPM_BUILD_ROOT}/BUILD"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${INSTALLER_LINUX_DIR}/overlay"
                "${RPM_BUILD_ROOT}/BUILD"
    )

    # Copy binaries, libraries, and fix RPATH
    linux_copy_files("${RPM_BUILD_ROOT}/BUILD" ${TARGET_NAME})

    # Build RPM package
    find_program(RPMBUILD_EXECUTABLE rpmbuild)
    if(RPMBUILD_EXECUTABLE)
        set(RPM_SOURCE_NAME "${WS_LINUX_PACKAGE_NAME}-${PROJECT_VERSION}-0.${RPM_BUILD_ARCH}.rpm")
        set(PACKAGE_ARCH "${RPM_FILENAME_ARCH}${DISTRO_SUFFIX}")
        ws_resolve_output_name("${WS_LINUX_OUTPUT_NAME}" _rpm_resolved_name)
        set(RPM_FINAL_NAME "${_rpm_resolved_name}.rpm")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Building ${DISTRO_NAME} RPM package..."
            COMMAND ${RPMBUILD_EXECUTABLE} -bb "${RPM_BUILD_ROOT}/SPECS/rpm.spec"
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

add_custom_target(init-build-exe
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUILD_EXE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_EXE_DIR}"
    COMMENT "Initializing build-exe directory..."
)

add_custom_target(deploy ALL)

if(BUILD_DEB)
    add_dependencies(package-deb init-build-exe)
    add_dependencies(deploy package-deb)
endif()
if(BUILD_RPM)
    add_dependencies(package-rpm-fedora init-build-exe)
    add_dependencies(deploy package-rpm-fedora)
endif()
if(BUILD_RPM_OPENSUSE)
    add_dependencies(package-rpm-opensuse init-build-exe)
    add_dependencies(deploy package-rpm-opensuse)
endif()
