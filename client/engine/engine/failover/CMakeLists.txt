target_sources(engine PRIVATE
    failover.cpp
    failover.h
    failoverretcode.cpp
    failoverretcode.h
    failovers/accessipsfailover.cpp
    failovers/accessipsfailover.h
    failovers/basefailover.h
    failovers/dgafailover.cpp
    failovers/dgafailover.h
    failovers/dynamicdomainfailover.cpp
    failovers/dynamicdomainfailover.h
    failovers/echfailover.cpp
    failovers/echfailover.h
    failovers/hardcodeddomainfailover.h
    failovers/randomdomainfailover.h
    ifailover.h
)

if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)
