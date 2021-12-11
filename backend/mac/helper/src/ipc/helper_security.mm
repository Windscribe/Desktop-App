#include "helper_security.h"
#include "logger.h"
#include "utils.h"
#include "executable_signature.h"
#include <boost/algorithm/string.hpp>
#include <libproc.h>
#import <Foundation/Foundation.h>

void HelperSecurity::reset()
{
    pid_validity_cache_.clear();
}

bool HelperSecurity::verifyProcessId(pid_t pid)
{
#if defined(USE_SIGNATURE_CHECK)
    const auto it = pid_validity_cache_.find(pid);
    if (it != pid_validity_cache_.end())
        return it->second;

    LOG("HelperSecurity::verifyProcessId: new PID %u", static_cast<unsigned int>(pid));
    return verifyProcessIdImpl(pid);
#else
    (void)pid;
    return true;
#endif
}

bool HelperSecurity::verifyProcessIdImpl(pid_t pid)
{
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) <= 0)
    {
        LOG("Failed to get app/bundle name for PID %i", pid);
        return false;
    }
    
    std::string app_name = pathbuf;
    
    std::vector<std::string> endings;
    // Check for a correct ending.
    endings.push_back("/Contents/MacOS/installer");
    endings.push_back("/WindscribeEngine.app/Contents/MacOS/WindscribeEngine");
    
    const auto app_name_length = app_name.length();

    // Check bundle name.
    bool bFoundBundleName = false;
    for (const auto &ending : endings)
    {
        const auto ending_length = ending.length();
        if (app_name_length >= ending_length &&
            app_name.compare(app_name_length - ending_length, ending_length, ending) == 0)
        {
            bFoundBundleName = true;
            break;
        }
    }
    
    if (!bFoundBundleName)
    {
        LOG("Invalid app/bundle name for PID %i: '%s'", pid, app_name.c_str());
        return false;
    }

    ExecutableSignature sigCheck;
    bool result = sigCheck.verify(app_name);

    if (!result) {
        LOG("Signature verification failed for PID %i, %s", pid, sigCheck.lastError().c_str());
    }

    pid_validity_cache_[pid] = result;
    return result;
}
