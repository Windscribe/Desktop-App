target_sources(engine PRIVATE
    keepalivemanager.cpp
    keepalivemanager.h
    pingmanager.cpp
    pingmanager.h
    pingstorage.cpp
    pingstorage.h
)

#if(DEFINED IS_BUILD_TESTS)
    #add_subdirectory(tests)
#endif(DEFINED IS_BUILD_TESTS)
