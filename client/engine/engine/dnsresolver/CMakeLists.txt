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
endif (WIN32)
#todo Mac/Linux
