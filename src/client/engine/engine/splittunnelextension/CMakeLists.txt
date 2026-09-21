if(APPLE)
target_sources(engine PRIVATE
    splittunnelextensionmanager_mac.h
    splittunnelextensionmanager_mac.mm
    splittunnelextensionstate_mac.h
    systemextensions_mac.h
    systemextensions_mac.mm
)
add_subdirectory(tests)
endif()
