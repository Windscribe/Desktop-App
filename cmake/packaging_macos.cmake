# ------------------------------------------------------------------------------
# macOS Packaging Configuration
# ------------------------------------------------------------------------------

add_custom_target(prep-installer-macos
    DEPENDS build-app
)

# Run macdeployqt on Windscribe.app
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")
if(MACDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running macdeployqt on Windscribe.app..."
        COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:Windscribe>" -always-overwrite
    )
else()
    message(WARNING "macdeployqt not found, Qt frameworks may not be properly deployed")
endif()

# Remove unused Qt frameworks from Windscribe.app
add_custom_command(TARGET prep-installer-macos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Removing unused Qt frameworks..."
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:Windscribe>/Contents/Frameworks/QtQml.framework" || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_DIR:Windscribe>/Contents/Frameworks/QtQuick.framework" || (exit 0)
)

# Sign Windscribe.app
# First sign with --deep to sign all nested components
add_custom_command(TARGET prep-installer-macos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Signing Windscribe.app..."
    COMMAND ${CODESIGN_EXECUTABLE}
            --deep
            --force
            --options runtime
            --timestamp
            --sign "Developer ID Application"
            "$<TARGET_BUNDLE_DIR:Windscribe>"
)
# Then sign the main executable with entitlements (only if provisioning profile exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/provisioning_profile/embedded.provisionprofile")
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Signing main executable with entitlements..."
        COMMAND ${CODESIGN_EXECUTABLE}
                --force
                --options runtime
                --timestamp
                --entitlements "${CMAKE_CURRENT_BINARY_DIR}/src/client/windscribe_engine.entitlements.configured"
                --sign "Developer ID Application"
                "$<TARGET_BUNDLE_DIR:Windscribe>/Contents/MacOS/Windscribe"
    )
endif()

# Notarize Windscribe.app
if(ENABLE_NOTARIZE)
    add_custom_command(TARGET prep-installer-macos POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Notarizing Windscribe.app..."
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/notarize.sh
                "${DEVELOPMENT_TEAM}"
                "$<TARGET_BUNDLE_DIR:Windscribe>/.."
                "Windscribe"
    )
endif()

# Create windscribe.tar.lzma directly in installer's Resources directory
add_custom_command(TARGET prep-installer-macos POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Creating windscribe.tar.lzma archive..."
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/src/installer/mac/installer/installer.app/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E remove -f
            "${CMAKE_BINARY_DIR}/src/installer/mac/installer/installer.app/Contents/Resources/windscribe.tar.lzma"
    COMMAND tar --lzma -cf
            "${CMAKE_BINARY_DIR}/src/installer/mac/installer/installer.app/Contents/Resources/windscribe.tar.lzma"
            -C "$<TARGET_BUNDLE_DIR:Windscribe>"
            .
)

# Installer packaging - creates DMG
if(BUILD_INSTALLER)
    add_custom_target(build-dmg ALL
        DEPENDS prep-installer-macos installer
    )

    # Run macdeployqt on installer.app
    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET build-dmg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Running macdeployqt on installer.app..."
            COMMAND ${MACDEPLOYQT_EXECUTABLE} "$<TARGET_BUNDLE_DIR:installer>" -always-overwrite
        )
    endif()

    # Remove unused Qt frameworks from installer.app
    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_DIR:installer>/Contents/Frameworks/QtQml.framework" || (exit 0)
        COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_DIR:installer>/Contents/Frameworks/QtQuick.framework" || (exit 0)
    )

    # Sign installer.app
    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Signing installer.app..."
        COMMAND ${CODESIGN_EXECUTABLE}
                --deep
                --force
                --options runtime
                --timestamp
                --sign "Developer ID Application"
                "$<TARGET_BUNDLE_DIR:installer>"
        COMMAND ${CODESIGN_EXECUTABLE} -v "$<TARGET_BUNDLE_DIR:installer>"
    )

    # Rename installer.app to WindscribeInstaller.app
    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Renaming installer.app to WindscribeInstaller.app..."
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BUILD_INSTALLER_FILES}"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${BUILD_INSTALLER_FILES}/WindscribeInstaller.app"
        COMMAND ${CMAKE_COMMAND} -E rename
                "$<TARGET_BUNDLE_DIR:installer>"
                "${BUILD_INSTALLER_FILES}/WindscribeInstaller.app"
    )

    # Notarize WindscribeInstaller.app (before creating DMG)
    if(ENABLE_NOTARIZE)
        add_custom_command(TARGET build-dmg POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Notarizing WindscribeInstaller.app..."
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/notarize.sh
                    "${DEVELOPMENT_TEAM}"
                    "${BUILD_INSTALLER_FILES}"
                    "WindscribeInstaller"
        )
    endif()

    # Create DMG with create-dmg
    find_program(CREATE_DMG_EXECUTABLE create-dmg)
    if(NOT CREATE_DMG_EXECUTABLE)
        message(FATAL_ERROR "create-dmg not found - cannot build DMG")
    endif()

    add_custom_command(TARGET build-dmg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Creating DMG with create-dmg..."
        COMMAND ${CMAKE_COMMAND} -E remove -f "${BUILD_EXE_DIR}/Windscribe_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}.dmg"
        COMMAND ${CREATE_DMG_EXECUTABLE}
                --volname "WindscribeInstaller"
                --volicon "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/mac/installer/resources/windscribe.icns"
                --background "${CMAKE_CURRENT_SOURCE_DIR}/src/installer/mac/dmg/installer_background.png"
                --window-pos 100 425
                --window-size 350 350
                --icon-size 64
                --icon "WindscribeInstaller.app" 175 192
                --hide-extension "WindscribeInstaller.app"
                --format ULMO
                "${BUILD_EXE_DIR}/Windscribe_${PROJECT_VERSION}${WINDSCRIBE_BUILD_SUFFIX}.dmg"
                "${BUILD_INSTALLER_FILES}/WindscribeInstaller.app"
        WORKING_DIRECTORY "${BUILD_INSTALLER_FILES}"
    )
endif()
