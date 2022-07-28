target_sources(engine PRIVATE
    areslibraryinit.cpp
    areslibraryinit.h
    dnsrequest.cpp
    dnsrequest.h
    dnsresolver.cpp
    dnsresolver.h
    dnsserversconfiguration.cpp
    dnsserversconfiguration.h
    dnsutils.h
)

if (WIN32)
    target_sources(engine PRIVATE
        dnsutils_win.cpp
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        dnsutils_mac.cpp
    )
elseif(UNIX)
    #todo linux
endif()
