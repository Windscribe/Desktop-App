#pragma once

#include <memory>
#include "authchecker_linux.h"
#include "authchecker_win.h"
#include "authchecker_mac.h"

namespace AuthCheckerFactory
{
    std::unique_ptr<IAuthChecker> createAuthChecker()
    {
#if defined(Q_OS_WIN)
        return std::unique_ptr<AuthChecker_win>(new AuthChecker_win);
#elif defined (Q_OS_MACOS)
        return std::unique_ptr<AuthChecker_mac>(new AuthChecker_mac); // NOLINT
#else
        return std::unique_ptr<AuthChecker_linux>(new AuthChecker_linux);
#endif
    }
}
