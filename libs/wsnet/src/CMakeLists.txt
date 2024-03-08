target_sources(wsnet PRIVATE
    wsnet.cpp
    advancedparameters.h
    connectstate.h
)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/private")
    add_subdirectory(private)
else()
    add_subdirectory(public)
endif()

add_subdirectory(dnsresolver)
add_subdirectory(utils)
add_subdirectory(httpnetworkmanager)
add_subdirectory(failover)
add_subdirectory(serverapi)
add_subdirectory(emergencyconnect)
add_subdirectory(pingmanager)
