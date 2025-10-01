target_sources(base PRIVATE
    imultipleaccountdetection.h
    multipleaccountdetectionfactory.cpp
    multipleaccountdetectionfactory.h
)

if (WIN32)
    target_sources(base PRIVATE
        multipleaccountdetection_win.cpp
        multipleaccountdetection_win.h
        secretvalue_win.cpp
        secretvalue_win.h
    )
elseif(UNIX)    #both Mac and Linux
    target_sources(base PRIVATE
        multipleaccountdetection_posix.cpp
        multipleaccountdetection_posix.h
    )
endif()
