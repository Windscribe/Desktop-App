target_sources(engine PRIVATE
    icmp_header.h
    ipv4_header.h
    keepalivemanager.cpp
    keepalivemanager.h
    ipinghost.h
    pinghost_curl.cpp
    pinghost_curl.h
    pinghost_tcp.cpp
    pinghost_tcp.h
    failedpinglogcontroller.cpp
    failedpinglogcontroller.h
    pingmanager.cpp
    pingmanager.h
    pinglog.cpp
    pinglog.h
    pingstorage.cpp
    pingstorage.h
    pingmultiplehosts.cpp
    pingmultiplehosts.h
)

if (WIN32)
    target_sources(engine PRIVATE
        pinghost_icmp_win.cpp
        pinghost_icmp_win.h
    )
elseif(UNIX)    # both Mac and Linux
    target_sources(engine PRIVATE
        pinghost_icmp_posix.cpp
        pinghost_icmp_posix.h
    )
endif()

if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)
