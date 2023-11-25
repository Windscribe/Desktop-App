target_sources(engine PRIVATE
    inetworkdetectionmanager.h
    waitfornetworkconnectivity.cpp
    waitfornetworkconnectivity.h
)

if (WIN32)
    target_sources(engine PRIVATE
        networkchangeworkerthread.cpp
        networkchangeworkerthread.h
        networkdetectionmanager_win.cpp
        networkdetectionmanager_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        networkdetectionmanager_mac.cpp
        networkdetectionmanager_mac.h
        reachability.h
        reachability.m
        reachabilityevents.h
        reachabilityevents.mm
    )
elseif(UNIX)
    target_sources(engine PRIVATE
        networkdetectionmanager_linux.cpp
        networkdetectionmanager_linux.h
        routemonitor_linux.cpp
        routemonitor_linux.h
    )
endif()
