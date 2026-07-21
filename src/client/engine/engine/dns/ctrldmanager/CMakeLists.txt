target_sources(engine PRIVATE
    ictrldmanager.h
)

if (WIN32)
    target_sources(engine PRIVATE
        ctrldmanager_win.cpp
        ctrldmanager_win.h
    )
else()
    target_sources(engine PRIVATE
        ctrldmanager_posix.cpp
        ctrldmanager_posix.h
    )
endif()

