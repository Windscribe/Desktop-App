target_sources(engine PRIVATE
    failover.cpp
    failover.h
    failoverretcode.cpp
    failoverretcode.h
    ifailover.h
    failovers/basefailover.h
    failovers/hardcodeddomainfailover.h
    failovers/dynamicdomainfailover.h
    failovers/dynamicdomainfailover.cpp
    failovers/randomdomainfailover.h
    failovers/accessipsfailover.h
    failovers/accessipsfailover.cpp
)

if(DEFINED IS_BUILD_TESTS)
   add_subdirectory(tests)
endif(DEFINED IS_BUILD_TESTS)
