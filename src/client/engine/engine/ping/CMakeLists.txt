target_sources(engine PRIVATE
    keepalivemanager.cpp
    keepalivemanager.h
    pingmanager.cpp
    pingmanager.h
    pingstorage.cpp
    pingstorage.h
)

#if(IS_BUILD_TESTS)
    #add_subdirectory(tests)
#endif(IS_BUILD_TESTS)
