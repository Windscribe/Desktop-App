# ------------------------------------------------------------------------------
# Windows Packaging Configuration
# ------------------------------------------------------------------------------

# Resolve output name templates
ws_resolve_output_name("${WS_WIN_OUTPUT_NAME}" WS_WIN_RESOLVED_NAME)
ws_resolve_output_name("${WS_WIN_SYMBOLS_NAME}" WS_WIN_RESOLVED_SYMBOLS_NAME)

# Copy app artifacts target - runs after build-app on Windows
if(BUILD_APP)
    set(COPY_ARTIFACTS_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_SYMBOLS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E echo "Copying Windows files to installer directory..."

        # Copy common files (licenses, etc.)
        COMMAND ${CMAKE_COMMAND} -E echo "Copying common files..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/src/client/client-common/licenses/open_source_licenses.txt"
                "${BUILD_INSTALLER_FILES}/open_source_licenses.txt"
    )

    # Copy additional files (drivers, configs, etc.)
    foreach(_entry ${WS_WIN_ADDITIONAL_FILES})
        string(REPLACE "|" ";" _pair "${_entry}")
        list(GET _pair 0 _src)
        list(GET _pair 1 _dest)
        if(_src MATCHES "^DIR:")
            string(SUBSTRING "${_src}" 4 -1 _src_dir)
            list(APPEND COPY_ARTIFACTS_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_directory "${_src_dir}" "${BUILD_INSTALLER_FILES}/${_dest}"
            )
        else()
            get_filename_component(_dest_dir "${BUILD_INSTALLER_FILES}/${_dest}" DIRECTORY)
            list(APPEND COPY_ARTIFACTS_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E make_directory "${_dest_dir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "${BUILD_INSTALLER_FILES}/${_dest}"
            )
        endif()
    endforeach()

    # Copy app target executables
    foreach(_target ${WS_WIN_APP_TARGETS})
        list(APPEND COPY_ARTIFACTS_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${_target}>" "${BUILD_INSTALLER_FILES}/"
        )
    endforeach()

    # Copy bundled helper binaries
    foreach(_entry ${WS_BUNDLED_HELPERS})
        string(REPLACE "|" ";" _pair "${_entry}")
        list(GET _pair 0 _src)
        list(GET _pair 1 _dest)
        list(APPEND COPY_ARTIFACTS_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_src}" "${BUILD_INSTALLER_FILES}/${_dest}"
        )
    endforeach()

    if(BUILD_SYMBOLS)
        list(APPEND COPY_ARTIFACTS_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E echo "Collecting symbol files..."
        )
        foreach(_target ${WS_WIN_PDB_TARGETS})
            list(APPEND COPY_ARTIFACTS_COMMANDS
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_PDB_FILE:${_target}>" "${BUILD_SYMBOLS_DIR}/"
            )
        endforeach()
    endif()

    add_custom_target(copy-app-artifacts ALL
        DEPENDS build-app
        ${COPY_ARTIFACTS_COMMANDS}
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

    set(WINDSCRIBE_7Z_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${INSTALLER_TYPE}/windows/installer/resources/${WS_PRODUCT_NAME_LOWER}.7z")

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
        COMMAND ${CMAKE_COMMAND} -E echo "Creating ${WS_PRODUCT_NAME_LOWER}.7z archive..."
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
    set(WINDSCRIBE_INSTALLER_7Z "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/${INSTALLER_TYPE}/windows/bootstrap/resources/${WS_PRODUCT_NAME_LOWER}installer.7z")

    if(BUILD_INSTALLER)
        file(REMOVE "${WINDSCRIBE_INSTALLER_7Z}")
        set(PREP_BOOTSTRAP_DEPENDS ${WS_WIN_INSTALLER_TARGET})
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
                "${CMAKE_BINARY_DIR}/src/installer/${INSTALLER_TYPE}/windows/installer/${WS_WIN_INSTALLER_TARGET}.exe"
                "${BUILD_BOOTSTRAP_FILES}/${WS_WIN_RESOLVED_NAME}.exe"

        # Create bootstrap 7z archive
        COMMAND ${CMAKE_COMMAND} -E remove -f "${WINDSCRIBE_INSTALLER_7Z}"
        COMMAND ${7ZIP_EXECUTABLE} a "${WINDSCRIBE_INSTALLER_7Z}" "${BUILD_BOOTSTRAP_FILES}/*" -y -bso0 -bsp0
    )

    add_custom_target(prep-bootstrap DEPENDS ${WINDSCRIBE_INSTALLER_7Z})

    # Make bootstrap depend on prep-bootstrap so the 7z file exists before building
    add_dependencies(${WS_WIN_BOOTSTRAP_TARGET} prep-bootstrap)
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
        add_dependencies(deploy ${WS_WIN_BOOTSTRAP_TARGET})
    endif()

    set(DEPLOY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUILD_EXE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_EXE_DIR}"

        COMMAND ${CMAKE_COMMAND} -E echo "Copying bootstrap and symbols to build-exe..."

        # Copy final bootstrap to build-exe
        COMMAND ${CMAKE_COMMAND} -E copy
                "${CMAKE_BINARY_DIR}/src/installer/${INSTALLER_TYPE}/windows/bootstrap/${WS_PRODUCT_NAME_LOWER}_installer.exe"
                "${BUILD_EXE_DIR}/${WS_WIN_RESOLVED_NAME}.exe"
    )

    if(BUILD_SYMBOLS)
        list(APPEND DEPLOY_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E echo "Creating symbols archive..."
            COMMAND ${7ZIP_EXECUTABLE} a "${BUILD_EXE_DIR}/${WS_WIN_RESOLVED_SYMBOLS_NAME}.zip" "${BUILD_SYMBOLS_DIR}/*" -y -bso0 -bsp0
        )
    endif()

    add_custom_command(TARGET deploy POST_BUILD
        ${DEPLOY_COMMANDS}
    )
endif()
