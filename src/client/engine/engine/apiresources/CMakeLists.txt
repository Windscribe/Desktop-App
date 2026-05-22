target_sources(engine PRIVATE
    apiutils.cpp
    apiutils.h
    myipmanager.cpp
    myipmanager.h
)

#if(IS_BUILD_TESTS)
    #add_subdirectory(tests)
#endif(IS_BUILD_TESTS)
