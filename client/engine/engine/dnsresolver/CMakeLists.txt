target_sources(engine PRIVATE
    areslibraryinit.cpp
    areslibraryinit.h
    dnsrequest.cpp
    dnsrequest.h
    idnsresolver.h
    dnsserversconfiguration.cpp
    dnsserversconfiguration.h
    dnsutils.h
)

if (WIN32)
    target_sources(engine PRIVATE
	dnsresolver_win.cpp
	dnsresolver_win.h
        dnsutils_win.cpp
    )
elseif(APPLE)
    target_sources(engine PRIVATE
	dnsresolver_posix.cpp
	dnsresolver_posix.h
        dnsutils_mac.cpp
    )
elseif(UNIX)
    target_sources(engine PRIVATE
	dnsresolver_posix.cpp
	dnsresolver_posix.h
        dnsutils_linux.cpp
    )
endif()


if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)

