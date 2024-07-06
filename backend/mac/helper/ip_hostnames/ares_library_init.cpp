#include "ares_library_init.h"

#include <iostream>
#include "ares.h"
#include "../logger.h"


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
			Logger::instance().out("ares_library_init failed: %s", ares_strerror(status));
			failedInit_ = true;
		} else {
			init_ = true;
		}
    }
}
