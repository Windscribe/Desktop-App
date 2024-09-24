if (APPLE)
    target_sources(gui PRIVATE
        permissionmonitor_mac.mm
        permissionmonitor_mac.h
    )
endif()
