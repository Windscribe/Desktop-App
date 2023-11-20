target_sources(engine PRIVATE
    failovers/accessipsfailover.cpp
    failovers/accessipsfailover.h
    basefailover.h
    failovers/dgafailover.cpp
    failovers/dgafailover.h
    failovers/dynamicdomainfailover.cpp
    failovers/dynamicdomainfailover.h
    failovers/echfailover.cpp
    failovers/echfailover.h
    failovers/hardcodeddomainfailover.h
    failovers/randomdomainfailover.h
    failovers/cdndomainfailover.h
    ifailovercontainer.h
    failovercontainer.cpp
    failovercontainer.h
)

if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)
