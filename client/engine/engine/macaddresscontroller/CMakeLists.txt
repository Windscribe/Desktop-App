target_sources(engine PRIVATE
    imacaddresscontroller.cpp
    imacaddresscontroller.h
)

if (WIN32)
    target_sources(engine PRIVATE
        macaddresscontroller_win.cpp
        macaddresscontroller_win.h

    )
endif (WIN32)
#todo mac/linux
