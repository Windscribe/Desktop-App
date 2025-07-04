target_sources(wsnet PRIVATE
    areslibraryinit.cpp
    areslibraryinit.h
    dnsresolver_cares.cpp
    dnsresolver_cares.h
    dnsservers.cpp
    dnsservers.h
)

if (APPLE AND NOT IOS)
    target_sources(wsnet PRIVATE
        getdnsconfig_mac.cpp
        getdnsconfig_mac.h
    )
endif()
