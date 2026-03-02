if(NOT TARGET wsnet::wsnet)
    include(FetchContent)
    FetchContent_Declare(wsnet
        GIT_REPOSITORY git@github.com:Windscribe/wsnet.git
        GIT_TAG        1.4.8
    )
    FetchContent_MakeAvailable(wsnet)
endif()
