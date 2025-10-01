target_sources(wsnet PRIVATE
    ipingmethod.h
    pingmanager.cpp
    pingmanager.h
    pingmethod_http.cpp
    pingmethod_http.h
)

if (WIN32)
    target_sources(wsnet PRIVATE
        pingmethod_icmp_win.cpp
        pingmethod_icmp_win.h
        eventcallbackmanager_win.cpp
        eventcallbackmanager_win.h
    )
elseif (APPLE)
    target_sources(wsnet PRIVATE
        pingmethod_icmp_apple.cpp
        pingmethod_icmp_apple.h
        apple/simple_ping.mm
        apple/simple_ping.h
        apple/pingmanager_apple.mm
        apple/pingmanager_apple.h
    )
else()
    target_sources(wsnet PRIVATE
        pingmethod_icmp_posix.cpp
        pingmethod_icmp_posix.h
        processmanager.cpp
        processmanager.h
    )
endif()

