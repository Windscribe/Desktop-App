# ------------------------------------------------------------------------------
# macOS Packaging Configuration
# ------------------------------------------------------------------------------

# Resolve output name template
ws_resolve_output_name("${WS_MAC_OUTPUT_NAME}" WS_MAC_RESOLVED_NAME)

add_custom_target(prep-installer-macos
    DEPENDS build-app
)

# Run macdeployqt on main application bundle
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")
if(MACDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running macdeployqt on ${WS_MAC_APP_BUNDLE_NAME}..."
        COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:${WS_APP_TARGET}>" -always-overwrite
    )
else()
    message(WARNING "macdeployqt not found, Qt frameworks may not be properly deployed")
endif()

# Remove unused Qt frameworks from main application bundle
foreach(_fw ${WS_MAC_UNUSED_FRAMEWORKS})
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_DIR:${WS_APP_TARGET}>/Contents/Frameworks/${_fw}.framework" || (exit 0)
    )
endforeach()

# Sign main application bundle
# First sign with --deep to sign all nested components
add_custom_command(TARGET prep-installer-macos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Signing ${WS_MAC_APP_BUNDLE_NAME}..."
    COMMAND ${CODESIGN_EXECUTABLE}
            --deep
            --force
            --options runtime
            --timestamp
            --sign "Developer ID Application"
            "$<TARGET_BUNDLE_DIR:${WS_APP_TARGET}>"
)
# Then sign the main executable with entitlements (only if provisioning profile exists)
if(WS_MAC_ENTITLEMENTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/provisioning_profile/embedded.provisionprofile")
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Signing main executable with entitlements..."
        COMMAND ${CODESIGN_EXECUTABLE}
                --force
                --options runtime
                --timestamp
                --entitlements "${WS_MAC_ENTITLEMENTS}"
                --sign "Developer ID Application"
                "$<TARGET_BUNDLE_DIR:${WS_APP_TARGET}>/Contents/MacOS/${WS_APP_TARGET}"
    )
endif()

# Notarize main application bundle
if(ENABLE_NOTARIZE)
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Notarizing ${WS_MAC_APP_BUNDLE_NAME}..."
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/notarize.sh
                "${DEVELOPMENT_TEAM}"
                "$<TARGET_BUNDLE_DIR:${WS_APP_TARGET}>/.."
                "${WS_APP_TARGET}"
    )
endif()

# Create compressed app archive in installer's Resources directory
set(_INSTALLER_RESOURCES "$<TARGET_BUNDLE_DIR:${WS_MAC_INSTALLER_TARGET}>/Contents/Resources")
add_custom_command(TARGET prep-installer-macos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Creating ${WS_PRODUCT_NAME_LOWER}.tar.lzma..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_INSTALLER_RESOURCES}"
    COMMAND ${CMAKE_COMMAND} -E remove -f "${_INSTALLER_RESOURCES}/${WS_PRODUCT_NAME_LOWER}.tar.lzma"
    COMMAND tar --lzma -cf
            "${_INSTALLER_RESOURCES}/${WS_PRODUCT_NAME_LOWER}.tar.lzma"
            -C "$<TARGET_BUNDLE_DIR:${WS_APP_TARGET}>"
            .
)

# Installer packaging - creates DMG
if(BUILD_INSTALLER)
    add_custom_target(build-dmg ALL
        DEPENDS prep-installer-macos ${WS_MAC_INSTALLER_TARGET}
    )

    # Run macdeployqt on installer.app
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET build-dmg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Running macdeployqt on installer.app..."
            COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:${WS_MAC_INSTALLER_TARGET}>" -always-overwrite
        )
    endif()

    # Remove unused Qt frameworks from installer.app
    foreach(_fw ${WS_MAC_UNUSED_FRAMEWORKS})
        add_custom_command(TARGET build-dmg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory
                    "$<TARGET_BUNDLE_DIR:${WS_MAC_INSTALLER_TARGET}>/Contents/Frameworks/${_fw}.framework" || (exit 0)
        )
    endforeach()

    # Sign installer.app
    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Signing installer.app..."
        COMMAND ${CODESIGN_EXECUTABLE}
                --deep
                --force
                --options runtime
                --timestamp
                --sign "Developer ID Application"
                "$<TARGET_BUNDLE_DIR:${WS_MAC_INSTALLER_TARGET}>"
        COMMAND ${CODESIGN_EXECUTABLE} -v "$<TARGET_BUNDLE_DIR:${WS_MAC_INSTALLER_TARGET}>"
    )

    # Rename installer.app to ${WS_MAC_INSTALLER_BUNDLE_NAME}
    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Renaming installer.app to ${WS_MAC_INSTALLER_BUNDLE_NAME}..."
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUILD_INSTALLER_FILES}/${WS_MAC_INSTALLER_BUNDLE_NAME}"
        COMMAND ${CMAKE_COMMAND} -E rename
                "$<TARGET_BUNDLE_DIR:${WS_MAC_INSTALLER_TARGET}>"
                "${BUILD_INSTALLER_FILES}/${WS_MAC_INSTALLER_BUNDLE_NAME}"
    )

    # Notarize ${WS_MAC_INSTALLER_BUNDLE_NAME} (before creating DMG)
    if(ENABLE_NOTARIZE)
        add_custom_command(TARGET build-dmg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Notarizing ${WS_MAC_INSTALLER_BUNDLE_NAME}..."
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/notarize.sh
                    "${DEVELOPMENT_TEAM}"
                    "${BUILD_INSTALLER_FILES}"
                    "${WS_MAC_INSTALLER_APP_NAME}"
        )
    endif()

    # Create DMG with dmgbuild
    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Creating DMG with dmgbuild..."
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUILD_EXE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_EXE_DIR}"
        COMMAND python3 -m dmgbuild
                -s "${WS_MAC_DMGBUILD_SETTINGS}"
                -D "app=${BUILD_INSTALLER_FILES}/${WS_MAC_INSTALLER_BUNDLE_NAME}"
                -D "background=${WS_MAC_DMGBUILD_BACKGROUND}"
                "${WS_MAC_INSTALLER_APP_NAME}"
                "${BUILD_EXE_DIR}/${WS_MAC_RESOLVED_NAME}.dmg"
        WORKING_DIRECTORY "${BUILD_INSTALLER_FILES}"
    )
endif()
