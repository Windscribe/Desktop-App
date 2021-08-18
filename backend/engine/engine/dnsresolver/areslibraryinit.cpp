#include "areslibraryinit.h"
#include "ares.h"
#include "utils/logger.h"

void AresLibraryInit::init()
{
    if (!bInitialized_)
    {
        if (!bFailedInitialize_)
        {
            int status = ares_library_init(ARES_LIB_INIT_ALL);
            if (status != ARES_SUCCESS)
            {
                qCDebug(LOG_BASIC) << "ares_library_init failed:" << QString::fromStdString(ares_strerror(status));
                bFailedInitialize_ = true;
            }
            else
            {
                bInitialized_ = true;
            }
        }
    }
}

AresLibraryInit::AresLibraryInit() : bInitialized_(false), bFailedInitialize_(false)
{
}

AresLibraryInit::~AresLibraryInit()
{
    if (bInitialized_)
    {
        ares_library_cleanup();
    }
}
