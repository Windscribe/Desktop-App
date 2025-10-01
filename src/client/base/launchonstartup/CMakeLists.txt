target_sources(base PRIVATE
    launchonstartup.cpp
    launchonstartup.h
)

if (WIN32)
    target_sources(base PRIVATE
        launchonstartup_win.cpp
        launchonstartup_win.h
    )
elseif(APPLE)
    target_sources(base PRIVATE
        launchonstartup_mac.mm
        launchonstartup_win.h
    )
elseif(UNIX)
    target_sources(base PRIVATE
        launchonstartup_linux.cpp
        launchonstartup_linux.h
    )
endif()
