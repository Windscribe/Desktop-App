target_sources(engine PRIVATE
    apiutils.cpp
    apiutils.h
    myipmanager.cpp
    myipmanager.h
)

#if(DEFINED IS_BUILD_TESTS)
    #add_subdirectory(tests)
#endif(DEFINED IS_BUILD_TESTS)
