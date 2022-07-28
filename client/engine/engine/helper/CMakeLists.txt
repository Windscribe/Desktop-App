target_sources(engine PRIVATE
    ihelper.h
    initializehelper.cpp
    initializehelper.h
)

if (WIN32)
    target_sources(engine PRIVATE
        helper_win.cpp
        helper_win.h
        windscribeinstallhelper_win.cpp
        windscribeinstallhelper_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        helper_mac.cpp
        helper_mac.h
        helper_posix.cpp
        helper_posix.h
        installhelper_mac.h
        installhelper_mac.mm
    )
elseif(UNIX)
    #todo linux
endif()

