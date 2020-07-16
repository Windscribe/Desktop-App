#include "ares_library_init.h"

#include <iostream>
#include "ares.h"
#include "../logger.h"

#pragma comment(lib, "libcares.lib")

AresLibraryInit::AresLibraryInit()
	: bInitialized_(false)
	, bFailedInitialize_(false)
{

}

AresLibraryInit::~AresLibraryInit()
{
	if (bInitialized_)
	{
		ares_library_cleanup();
	}
}

void AresLibraryInit::init()
{
	if (!bInitialized_)
	{
		if (!bFailedInitialize_)
		{
			int status = ares_library_init(ARES_LIB_INIT_ALL);
			if (status != ARES_SUCCESS)
			{
				Logger::instance().out("ares_library_init failed: %s", ares_strerror(status));
				bFailedInitialize_ = true;
			}
			else
			{
				bInitialized_ = true;
			}
		}
	}
}
