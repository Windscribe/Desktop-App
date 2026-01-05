# ------------------------------------------------------------------------------
# Windows Packaging Configuration
# ------------------------------------------------------------------------------

# Define architecture-specific suffix
set(ARCH_SUFFIX "amd64")
set(WINDSCRIBE_ARCH_SUFFIX "_amd64")
if(BUILD_ARM64)
    set(ARCH_SUFFIX "arm64")
    set(WINDSCRIBE_ARCH_SUFFIX "_arm64")
endif()

# Copy app artifacts target - runs after build-app on Windows
if(BUILD_APP)
    add_custom_target(copy-app-artifacts ALL
        DEPENDS build-app
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_SYMBOLS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E echo "Copying Windows files to installer directory..."

        # Copy main executables
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/client/Windscribe.exe" "${BUILD_INSTALLER_FILES}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/windscribe-cli/windscribe-cli.exe" "${BUILD_INSTALLER_FILES}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/helper/windows/WindscribeService.exe" "${BUILD_INSTALLER_FILES}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/utils/windows/windscribe_install_helper/WindscribeInstallHelper.exe" "${BUILD_INSTALLER_FILES}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/utils/windows/wireguard_service/WireguardService.exe" "${BUILD_INSTALLER_FILES}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/installer/windows/uninstaller/uninstall.exe" "${BUILD_INSTALLER_FILES}/"

        # Copy Qt config file
        COMMAND ${CMAKE_COMMAND} -E echo "Copying Qt configuration..."
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/additional_files/qt"
                "${BUILD_INSTALLER_FILES}/qt"

        # Copy custom dependencies (WireGuard, WSTunnel, Wintun, OpenVPN, ctrld, OpenVPN DCO)
        COMMAND ${CMAKE_COMMAND} -E echo "Copying custom dependencies..."

        # OpenVPN
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/openvpn/openvpn.exe"
                "${BUILD_INSTALLER_FILES}/windscribeopenvpn.exe"

        # ctrld
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/ctrld/ctrld.exe"
                "${BUILD_INSTALLER_FILES}/windscribectrld.exe"

        # WireGuard
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/wireguard/tunnel.dll"
                "${BUILD_INSTALLER_FILES}/tunnel.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/wireguard/wireguard.dll"
                "${BUILD_INSTALLER_FILES}/wireguard.dll"

        # WSTunnel
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/wstunnel/windscribewstunnel.exe"
                "${BUILD_INSTALLER_FILES}/windscribewstunnel.exe"

        # Wintun
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/wintun/bin/wintun.dll"
                "${BUILD_INSTALLER_FILES}/wintun.dll"

        # OpenVPN DCO drivers
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}/openvpndco/win10"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}/openvpndco/win11"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win10/ovpn-dco.cat"
                "${BUILD_INSTALLER_FILES}/openvpndco/win10/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win10/ovpn-dco.inf"
                "${BUILD_INSTALLER_FILES}/openvpndco/win10/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win10/ovpn-dco.sys"
                "${BUILD_INSTALLER_FILES}/openvpndco/win10/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win11/ovpn-dco.cat"
                "${BUILD_INSTALLER_FILES}/openvpndco/win11/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win11/ovpn-dco.inf"
                "${BUILD_INSTALLER_FILES}/openvpndco/win11/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WINDSCRIBE_BUILD_LIBS_PATH}/ovpn-dco-win/win11/ovpn-dco.sys"
                "${BUILD_INSTALLER_FILES}/openvpndco/win11/"

        # Copy architecture-specific files (devcon, tapctl, split tunnel driver)
        COMMAND ${CMAKE_COMMAND} -E echo "Copying architecture-specific files..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/additional_files/driver_utils/${ARCH_SUFFIX}/devcon.exe"
                "${BUILD_INSTALLER_FILES}/devcon.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/additional_files/driver_utils/${ARCH_SUFFIX}/tapctl.exe"
                "${BUILD_INSTALLER_FILES}/tapctl.exe"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}/splittunnel"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/additional_files/splittunnel/${ARCH_SUFFIX}/windscribesplittunnel.cat"
                "${BUILD_INSTALLER_FILES}/splittunnel/windscribesplittunnel.cat"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/additional_files/splittunnel/${ARCH_SUFFIX}/windscribesplittunnel.inf"
                "${BUILD_INSTALLER_FILES}/splittunnel/windscribesplittunnel.inf"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/additional_files/splittunnel/${ARCH_SUFFIX}/windscribesplittunnel.sys"
                "${BUILD_INSTALLER_FILES}/splittunnel/windscribesplittunnel.sys"

        # Copy common files (licenses, etc.)
        COMMAND ${CMAKE_COMMAND} -E echo "Copying common files..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/licenses/open_source_licenses.txt"
                "${BUILD_INSTALLER_FILES}/open_source_licenses.txt"

        # Collect symbol files (PDB)
        COMMAND ${CMAKE_COMMAND} -E echo "Collecting symbol files..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/client/Windscribe.pdb" "${BUILD_SYMBOLS_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/windscribe-cli/windscribe-cli.pdb" "${BUILD_SYMBOLS_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/helper/windows/WindscribeService.pdb" "${BUILD_SYMBOLS_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/utils/windows/windscribe_install_helper/WindscribeInstallHelper.pdb" "${BUILD_SYMBOLS_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/src/utils/windows/wireguard_service/WireguardService.pdb" "${BUILD_SYMBOLS_DIR}/"
    )
endif()

find_program(7ZIP_EXECUTABLE 7z PATHS "${CMAKE_CURRENT_SOURCE_DIR}/tools/bin" NO_DEFAULT_PATH)
if(NOT 7ZIP_EXECUTABLE)
    find_program(7ZIP_EXECUTABLE 7z)
endif()
if(NOT 7ZIP_EXECUTABLE)
    message(FATAL_ERROR "7z executable not found. Please install 7-Zip or place 7z.exe in ${CMAKE_CURRENT_SOURCE_DIR}/tools/bin/")
endif()

# Installer packaging - creates 7z archives from existing files
if(BUILD_INSTALLER)
    file(MAKE_DIRECTORY "${BUILD_SYMBOLS_DIR}")
    file(MAKE_DIRECTORY "${BUILD_BOOTSTRAP_FILES}")

    set(WINDSCRIBE_7Z_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/installer/resources/windscribe.7z")

    if(BUILD_APP)
        file(REMOVE "${WINDSCRIBE_7Z_FILE}")
        set(PACKAGE_APP_DEPENDS copy-app-artifacts)
    else()
        set(PACKAGE_APP_DEPENDS "")
    endif()

    add_custom_command(
        OUTPUT ${WINDSCRIBE_7Z_FILE}
        DEPENDS ${PACKAGE_APP_DEPENDS}
        # Create 7z archive (needed by installer build)
        COMMAND ${CMAKE_COMMAND} -E echo "Creating windscribe.7z archive..."
        COMMAND ${CMAKE_COMMAND} -E remove -f "${WINDSCRIBE_7Z_FILE}"
        COMMAND ${7ZIP_EXECUTABLE} a "${WINDSCRIBE_7Z_FILE}" "${BUILD_INSTALLER_FILES}/*" -y -bso0 -bsp0
    )

    # Create custom target that depends on the outputs
    add_custom_target(prep-installer-windows DEPENDS ${WINDSCRIBE_7Z_FILE})
    if(TARGET sign-app)
        add_dependencies(prep-installer-windows sign-app)
    endif()
endif()

# Bootstrap packaging - packages installer.exe and builds bootstrap
if(BUILD_BOOTSTRAP)
    set(WINDSCRIBE_INSTALLER_7Z "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/bootstrap/resources/windscribeinstaller.7z")

    if(BUILD_INSTALLER)
        file(REMOVE "${WINDSCRIBE_INSTALLER_7Z}")
        set(PREP_BOOTSTRAP_DEPENDS installer)
        if(TARGET sign-installer)
            list(APPEND PREP_BOOTSTRAP_DEPENDS sign-installer)
        endif()
    else()
        set(PREP_BOOTSTRAP_DEPENDS "")
    endif()

    add_custom_command(
        OUTPUT ${WINDSCRIBE_INSTALLER_7Z}
        DEPENDS ${PREP_BOOTSTRAP_DEPENDS}
        COMMAND ${CMAKE_COMMAND} -E echo "Packaging installer for bootstrap..."

        # Copy installer.exe to bootstrap directory with version name
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_BINARY_DIR}/src/installer/windows/installer/installer.exe"
                "${BUILD_BOOTSTRAP_FILES}/Windscribe_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}${WINDSCRIBE_ARCH_SUFFIX}.exe"

        # Create bootstrap 7z archive
        COMMAND ${CMAKE_COMMAND} -E remove -f "${WINDSCRIBE_INSTALLER_7Z}"
        COMMAND ${7ZIP_EXECUTABLE} a "${WINDSCRIBE_INSTALLER_7Z}" "${BUILD_BOOTSTRAP_FILES}/*" -y -bso0 -bsp0
    )

    add_custom_target(prep-bootstrap DEPENDS ${WINDSCRIBE_INSTALLER_7Z})

    # Make bootstrap depend on prep-bootstrap so the 7z file exists before building
    add_dependencies(bootstrap prep-bootstrap)
endif()

# Deploy target - copy bootstrap and symbols to build-exe (runs for both build and sign)
if(BUILD_BOOTSTRAP OR SIGN_BOOTSTRAP)
    add_custom_target(deploy ALL)

    if(SIGN_BOOTSTRAP)
        # When signing, deploy should run AFTER sign-bootstrap
        # Note: We can't use if(TARGET sign-bootstrap) because sign_bootstrap.cmake
        # is included after this file, so the target doesn't exist yet
        add_dependencies(deploy sign-bootstrap)
    elseif(BUILD_BOOTSTRAP)
        # When building (not signing), deploy runs after bootstrap
        add_dependencies(deploy bootstrap)
    endif()

    add_custom_command(TARGET deploy POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Copying bootstrap and symbols to build-exe..."

        # Remove old file first to ensure fresh copy
        COMMAND ${CMAKE_COMMAND} -E remove -f "${BUILD_EXE_DIR}/Windscribe_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}${WINDSCRIBE_ARCH_SUFFIX}.exe"

        # Copy final bootstrap to build-exe
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/src/installer/windows/bootstrap/windscribe_installer.exe"
                "${BUILD_EXE_DIR}/Windscribe_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}${WINDSCRIBE_ARCH_SUFFIX}.exe"

        # Create symbols archive and copy to build-exe
        COMMAND ${CMAKE_COMMAND} -E echo "Creating symbols archive..."
        COMMAND ${7ZIP_EXECUTABLE} a "${BUILD_EXE_DIR}/WindscribeSymbols_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}${WINDSCRIBE_ARCH_SUFFIX}.zip" "${BUILD_SYMBOLS_DIR}/*" -y -bso0 -bsp0
    )
endif()
