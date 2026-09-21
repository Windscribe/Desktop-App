target_sources(engine PRIVATE
    isleepevents.h
)

if (WIN32)
    target_sources(engine PRIVATE
        sleepevents_win.cpp
        sleepevents_win.h
    )
elseif(APPLE)
    target_sources(engine PRIVATE
        sleepevents_mac.h
        sleepevents_mac.mm
    )
endif()
