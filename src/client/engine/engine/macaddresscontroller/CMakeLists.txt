target_sources(engine PRIVATE
    imacaddresscontroller.cpp
    imacaddresscontroller.h
)

if (WIN32)
    target_sources(engine PRIVATE
        macaddresscontroller_win.cpp
        macaddresscontroller_win.h

    )
elseif(UNIX AND NOT APPLE)
    target_sources(engine PRIVATE
        macaddresscontroller_linux.cpp
        macaddresscontroller_linux.h
    )
endif()
