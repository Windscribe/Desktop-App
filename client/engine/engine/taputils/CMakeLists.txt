if (WIN32)
    target_sources(engine PRIVATE
        checkadapterenable.cpp
        checkadapterenable.h
        tapinstall_win.cpp
        tapinstall_win.h
    )
endif (WIN32)
