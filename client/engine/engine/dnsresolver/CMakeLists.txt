target_sources(engine PRIVATE
    areslibraryinit.cpp
    areslibraryinit.h
    dnsresolver_cares.cpp
    dnsresolver_cares.h
    dnsrequest.cpp
    dnsrequest.h
    idnsresolver.h
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
    target_sources(engine PRIVATE
        dnsutils_linux.cpp
    )
endif()


if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)

