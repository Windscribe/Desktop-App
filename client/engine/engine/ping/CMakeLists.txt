target_sources(engine PRIVATE
    icmp_header.h
    ipv4_header.h
    keepalivemanager.cpp
    keepalivemanager.h
    pinghost.cpp
    pinghost.h
    pinghost_tcp.cpp
    pinghost_tcp.h
)

if (WIN32)
    target_sources(engine PRIVATE
        pinghost_icmp_win.cpp
        pinghost_icmp_win.h
    )
elseif(UNIX)    # both Mac and Linux
    target_sources(engine PRIVATE
        pinghost_icmp_mac.cpp       # todo rename to _posix
        pinghost_icmp_mac.h
    )
endif()
