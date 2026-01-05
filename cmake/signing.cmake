# ------------------------------------------------------------------------------
# Common Code Signing Configuration
# ------------------------------------------------------------------------------

# Common build directories
set(BUILD_EXE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build-exe")
set(BUILD_TEMP_DIR "${CMAKE_BINARY_DIR}/temp")
set(BUILD_INSTALLER_FILES "${BUILD_TEMP_DIR}/InstallerFiles")

# ------------------------------------------------------------------------------
# Windows Signing Configuration
# ------------------------------------------------------------------------------

if(WIN32)
    set(SIGNTOOL "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/signing/signtool.exe")
    set(SIGNING_CERT "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/windows/signing/code_signing.der")
    set(SIGNING_TIMESTAMP "http://timestamp.digicert.com")
    set(WINDOWS_CERT_SUBJECT_NAME "Windscribe Limited")

    set(WINDOWS_SIGN_PARAMS "/fd SHA256 /td SHA256 /tr ${SIGNING_TIMESTAMP} /f ${SIGNING_CERT} /csp \"eToken Base Cryptographic Provider\" /kc \"[{{%WINDOWS_SIGNING_TOKEN_PASSWORD%}}]=%CONTAINER_NAME%\" /n \"${WINDOWS_CERT_SUBJECT_NAME}\"")

    if(SIGN_APP OR SIGN_INSTALLER OR SIGN_BOOTSTRAP)
        if(NOT EXISTS "${SIGNTOOL}")
            message(FATAL_ERROR "signtool not found at ${SIGNTOOL}. Cannot sign.")
        endif()

        if(NOT EXISTS "${SIGNING_CERT}")
            message(FATAL_ERROR "Signing certificate not found at ${SIGNING_CERT}. Cannot sign.")
        endif()

        if(NOT DEFINED ENV{WINDOWS_SIGNING_TOKEN_PASSWORD})
            message(FATAL_ERROR "WINDOWS_SIGNING_TOKEN_PASSWORD environment variable is not set. Cannot sign.")
        endif()
    endif()

    if(SIGN_APP)
        add_custom_target(sign-app ALL)
        if(TARGET copy-app-artifacts)
            add_dependencies(sign-app copy-app-artifacts)
        endif()

        add_custom_command(TARGET sign-app
            COMMAND @for %f in (${BUILD_INSTALLER_FILES}/*.exe ${BUILD_INSTALLER_FILES}/*.dll) do @${SIGNTOOL} sign ${WINDOWS_SIGN_PARAMS} \"${BUILD_INSTALLER_FILES}/%~nxf\" >nul 2>&1
            COMMENT "Signing Windows executables and DLLs..."
        )
    endif()

    if(SIGN_INSTALLER)
        add_custom_target(sign-installer ALL)
        if(TARGET installer)
            add_dependencies(sign-installer installer)
        endif()

        add_custom_command(TARGET sign-installer
            COMMAND ${CMAKE_COMMAND} -E echo "Signing installer.exe..."
            COMMAND ${SIGNTOOL} sign ${WINDOWS_SIGN_PARAMS} ${CMAKE_BINARY_DIR}/src/installer/windows/installer/installer.exe
        )
    endif()

    if(SIGN_BOOTSTRAP)
        add_custom_target(sign-bootstrap ALL)
        if(TARGET bootstrap)
            add_dependencies(sign-bootstrap bootstrap)
        endif()

        add_custom_command(TARGET sign-bootstrap
            COMMAND ${SIGNTOOL} sign ${WINDOWS_SIGN_PARAMS} ${CMAKE_BINARY_DIR}/src/installer/windows/bootstrap/windscribe_installer.exe
            COMMENT "Signing windscribe_installer.exe..."
        )
    endif()
endif()

# ------------------------------------------------------------------------------
# macOS Signing Configuration
# ------------------------------------------------------------------------------

if(APPLE)
    # Extract Development Team ID from executable_signature_defs.h
    if(NOT DEFINED DEVELOPMENT_TEAM OR DEVELOPMENT_TEAM STREQUAL "")
        set(SIGNATURE_DEFS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/client/common/utils/executable_signature/executable_signature_defs.h")
        if(EXISTS "${SIGNATURE_DEFS_FILE}")
            file(READ "${SIGNATURE_DEFS_FILE}" SIGNATURE_DEFS)
            if(SIGNATURE_DEFS MATCHES "MACOS_CERT_DEVELOPER_ID \"[^(]+\\(([A-Z0-9]+)\\)\"")
                set(DEVELOPMENT_TEAM "${CMAKE_MATCH_1}" CACHE STRING "Apple Development Team ID")
                message(STATUS "Extracted DEVELOPMENT_TEAM: ${DEVELOPMENT_TEAM}")
            else()
                message(FATAL_ERROR "Could not extract DEVELOPMENT_TEAM from ${SIGNATURE_DEFS_FILE}")
            endif()
        else()
            message(FATAL_ERROR "Could not find ${SIGNATURE_DEFS_FILE}")
        endif()
    endif()

    # Find codesign utility
    find_program(CODESIGN_EXECUTABLE codesign)
    if(NOT CODESIGN_EXECUTABLE)
        message(FATAL_ERROR "codesign not found. Code signing is required on macOS.")
    endif()
endif()
