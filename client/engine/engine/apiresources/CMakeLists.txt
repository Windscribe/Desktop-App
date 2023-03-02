target_sources(engine PRIVATE
    apiresourcesmanager.cpp
    apiresourcesmanager.h
    checkupdatemanager.cpp
    checkupdatemanager.h
    myipmanager.cpp
    myipmanager.h
)

if(DEFINED IS_BUILD_TESTS)
   #add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)
