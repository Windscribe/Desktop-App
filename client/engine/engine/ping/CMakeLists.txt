target_sources(engine PRIVATE
    keepalivemanager.cpp
    keepalivemanager.h
    failedpinglogcontroller.cpp
    failedpinglogcontroller.h
    pingmanager.cpp
    pingmanager.h
    pinglog.cpp
    pinglog.h
    pingstorage.cpp
    pingstorage.h
)

#if(DEFINED IS_BUILD_TESTS)
    #add_subdirectory(tests)
#endif(DEFINED IS_BUILD_TESTS)
