target_sources(engine PRIVATE
    myipmanager.cpp
    myipmanager.h
)

#if(DEFINED IS_BUILD_TESTS)
    #add_subdirectory(tests)
#endif(DEFINED IS_BUILD_TESTS)
