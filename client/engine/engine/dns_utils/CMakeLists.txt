target_sources(engine PRIVATE
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


