target_sources(engine PRIVATE
    convert_utils.h
    ihelperbackend.h
    helper_base.h
    helper.h
    initializehelper.cpp
    initializehelper.h
)

if (WIN32)
    target_sources(engine PRIVATE
        helperbackend_win.cpp
        helperbackend_win.h
        helper_win.cpp
        helper_win.h
        installhelper_win.cpp
        installhelper_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        helperbackend_mac.cpp
        helperbackend_mac.h
        helper_mac.cpp
        helper_mac.h
        helper_posix.cpp
        helper_posix.h
        installhelper_mac.h
        installhelper_mac.mm
    )
elseif(UNIX)
    target_sources(engine PRIVATE
        helperbackend_linux.cpp
        helperbackend_linux.h
        helper_linux.cpp
        helper_linux.h
        helper_posix.cpp
        helper_posix.h
    )
endif()

