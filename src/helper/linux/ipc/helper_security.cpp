#include <limits.h>
#include <fstream>
#include <grp.h>
#include <sstream>
#include <sys/types.h>
#include <spdlog/spdlog.h>

#include "helper_security.h"

#include "utils/executable_signature/executable_signature.h"

bool HelperSecurity::verifySignature()
{
#if defined(USE_SIGNATURE_CHECK)
    ExecutableSignature sigCheck;
    bool result = sigCheck.verify("/opt/windscribe/Windscribe");

    if (!result) {
        spdlog::warn("Signature verification failed for Windscribe: {}", sigCheck.lastError());
    }
    return result;
#else
    return true;
#endif
}
