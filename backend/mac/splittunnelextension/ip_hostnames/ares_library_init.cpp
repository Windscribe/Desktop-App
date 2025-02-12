#include "ares_library_init.h"

#include <iostream>
#include <spdlog/spdlog.h>
#include "ares.h"

AresLibraryInit::AresLibraryInit() : init_(false), failedInit_(false)
{
}

AresLibraryInit::~AresLibraryInit()
{
    if (init_) {
        ares_library_cleanup();
    }
}

void AresLibraryInit::init()
{
    if (!init_ && !failedInit_) {
        int status = ares_library_init(ARES_LIB_INIT_ALL);
        if (status != ARES_SUCCESS) {
            spdlog::error("ares_library_init failed: {}", ares_strerror(status));
            failedInit_ = true;
        } else {
            init_ = true;
        }
    }
}
