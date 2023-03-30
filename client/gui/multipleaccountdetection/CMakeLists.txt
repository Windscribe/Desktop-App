target_sources(gui PRIVATE
    imultipleaccountdetection.h
    multipleaccountdetectionfactory.cpp
    multipleaccountdetectionfactory.h
)

if (WIN32)
    target_sources(gui PRIVATE
        multipleaccountdetection_win.cpp
        multipleaccountdetection_win.h
        secretvalue_win.cpp
        secretvalue_win.h
    )
elseif(UNIX)    #both Mac and Linux
    target_sources(gui PRIVATE
        multipleaccountdetection_posix.cpp
        multipleaccountdetection_posix.h
    )
endif()
