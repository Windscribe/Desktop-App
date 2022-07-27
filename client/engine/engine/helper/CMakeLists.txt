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
endif (WIN32)
#todo mac/linux
