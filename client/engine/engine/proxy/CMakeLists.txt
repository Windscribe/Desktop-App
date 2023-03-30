target_sources(engine PRIVATE
    proxyservercontroller.cpp
    proxyservercontroller.h
)

if (WIN32)
    target_sources(engine PRIVATE
        autodetectproxy_win.cpp
        autodetectproxy_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        autodetectproxy_mac.h
        autodetectproxy_mac.mm
        pmachelpers.h
        pmachelpers.mm
    )
elseif(UNIX)
    #todo linux
endif()
