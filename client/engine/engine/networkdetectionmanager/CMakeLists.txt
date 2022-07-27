target_sources(engine PRIVATE
    inetworkdetectionmanager.h
)

if (WIN32)
    target_sources(engine PRIVATE
        networkchangeworkerthread.cpp
        networkchangeworkerthread.h
        networkdetectionmanager_win.cpp
        networkdetectionmanager_win.h
    )
endif (WIN32)
#todo mac/linux
