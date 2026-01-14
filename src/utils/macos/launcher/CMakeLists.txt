cmake_minimum_required(VERSION 3.23)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(launcher_mac)

set(SOURCES
    appdelegate.h
    main.m
    appdelegate.mm
)

add_executable(WindscribeLauncher MACOSX_BUNDLE ${SOURCES})

target_compile_options(WindscribeLauncher PRIVATE "-fobjc-arc")

target_link_libraries(WindscribeLauncher
                      PRIVATE
                      "-framework AppKit"
)

set_target_properties(WindscribeLauncher
                      PROPERTIES
                      MACOSX_BUNDLE TRUE
                      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/info.plist
)
target_include_directories(WindscribeLauncher PRIVATE
                           ../../../client/common
)

# Sign launcher immediately after building
add_custom_command(TARGET WindscribeLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Signing WindscribeLauncher..."
    COMMAND ${CODESIGN_EXECUTABLE}
            --force
            --options runtime
            --timestamp
            --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/windscribelauncher.entitlements"
            --sign "Developer ID Application"
            $<TARGET_BUNDLE_DIR:WindscribeLauncher>
    COMMAND ${CODESIGN_EXECUTABLE} -v $<TARGET_BUNDLE_DIR:WindscribeLauncher>
)

