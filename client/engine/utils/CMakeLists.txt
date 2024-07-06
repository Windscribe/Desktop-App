target_sources(engine PRIVATE
    boost_includes.h
)
if (WIN32)
    target_sources(engine PRIVATE
        bfe_service_win.cpp
        bfe_service_win.h
        installedantiviruses_win.cpp
        installedantiviruses_win.h
        ras_service_win.cpp
        ras_service_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        interfaceutils_mac.cpp
        interfaceutils_mac.h
    )
elseif(UNIX)
    target_sources(engine PRIVATE
        dnsscripts_linux.cpp
        dnsscripts_linux.h
)
endif()
