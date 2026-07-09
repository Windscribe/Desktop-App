target_sources(engine PRIVATE
    connectivitydiagnostic.cpp
    connectivitydiagnostic.h
    connectivitydiagnosticcollector_common.cpp
    connectivitydiagnosticcollector_common.h
    connectivitydiagnosticresult.h
    dnsprobes.cpp
    dnsprobes.h
    httpprobes.cpp
    httpprobes.h
    iconnectivitydiagnosticcollector.h
)

if (WIN32)
    target_sources(engine PRIVATE
        connectivitydiagnosticcollector_win.cpp
        connectivitydiagnosticcollector_win.h
    )
elseif (APPLE)
    target_sources(engine PRIVATE
        connectivitydiagnosticcollector_mac.cpp
        connectivitydiagnosticcollector_mac.h
    )
elseif (UNIX AND NOT APPLE)
    target_sources(engine PRIVATE
        connectivitydiagnosticcollector_linux.cpp
        connectivitydiagnosticcollector_linux.h
    )
endif()
