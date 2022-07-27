target_sources(gui PRIVATE
    launchonstartup.cpp
    launchonstartup.h
)

if (WIN32)
    target_sources(gui PRIVATE
        launchonstartup_win.cpp
        launchonstartup_win.h
    )
endif (WIN32)

