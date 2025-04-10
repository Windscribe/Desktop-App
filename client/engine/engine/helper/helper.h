#pragma once

#include <QtSystemDetection>

#ifdef Q_OS_WIN
    #include "helper_win.h"
    typedef Helper_win Helper;

#elif defined(Q_OS_MACOS)
    #include "helper_mac.h"
    typedef Helper_mac Helper;

#elif defined(Q_OS_LINUX)
    #include "helper_linux.h"
    typedef Helper_linux Helper;
#endif
