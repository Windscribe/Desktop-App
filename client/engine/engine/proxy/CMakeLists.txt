target_sources(engine PRIVATE
    proxyservercontroller.cpp
    proxyservercontroller.h
    proxysettings.cpp
    proxysettings.h
)

if (WIN32)
    target_sources(engine PRIVATE
        autodetectproxy_win.cpp
        autodetectproxy_win.h
    )
endif (WIN32)
#todo mac/linux
