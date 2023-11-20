#include <limits.h>
#include <fstream>
#include <sstream>

#include "helper_security.h"

#if defined(USE_SIGNATURE_CHECK)
#include "boost/filesystem/path.hpp"

#include "../logger.h"

#include "../../../client/common/utils/executable_signature/executable_signature.h"

// Expects symLink to reference /path/*/exe, where * can be 'self', or a pid, or
// an exe name.
static std::string getProcessPath(const char* symLink)
{
    std::string result;
    result.resize(128);

    ssize_t rlin_size = ::readlink(symLink, &result[0], result.size() - 1);
    while (rlin_size == static_cast<int>(result.size() - 1))
    {
        // readlink(2) will fill our buffer and not necessarily terminate with NUL;
        if (result.size() >= PATH_MAX)
        {
            errno = ENAMETOOLONG;
            result.clear();
            return result;
        }

        result.resize(result.size() * 2);
        rlin_size = ::readlink(symLink, &result[0], result.size() - 1);
    }

    if (rlin_size == -1) {
        result.clear();
    }
    else {
        result.resize(rlin_size);
    }

    return result;
}

// Returns the full path and name of the executable associated with this pid, or an
// empty string if an error occurs.
static std::string getProcessPath(pid_t pid)
{
    std::ostringstream stream;
    stream << "/proc/" << pid << "/exe";
    std::string symLink = stream.str();

    return getProcessPath(symLink.c_str());
}

// Returns the directory that contains the application executable.
static std::string applicationDirPath()
{
    std::string procPath = getProcessPath("/proc/self/exe");

    if (!procPath.empty())
    {
        boost::filesystem::path path(procPath);
        if (path.has_parent_path()) {
            return path.parent_path().native();
        }
    }

    return std::string();
}
#endif

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
#endif

    return verifyProcessIdImpl(pid);
}

bool HelperSecurity::verifyProcessIdImpl(pid_t pid)
{
#if defined(USE_SIGNATURE_CHECK)
    //Logger::instance().out("Getting exe path and name for PID %i", pid);

    const std::string clientAppPath = getProcessPath(pid);
    if (clientAppPath.empty())
    {
        Logger::instance().out("Failed to get exe path and name for PID %i, errno %d", pid, errno);
        pid_validity_cache_[pid] = false;
        return false;
    }

    const std::string engineExePath = applicationDirPath() + "/Windscribe";

    //Logger::instance().out("Checking exe path matches engine's: %s", clientAppPath.c_str());

    if (engineExePath.compare(clientAppPath) != 0)
    {
        Logger::instance().out("Invalid calling application for PID %i, %s", pid, clientAppPath.c_str());
        pid_validity_cache_[pid] = false;
        return false;
    }

    //Logger::instance().out("Verifying signature...");

    ExecutableSignature sigCheck;
    bool result = sigCheck.verify(engineExePath);

    if (!result) {
        Logger::instance().out("Signature verification failed for PID %i, %s", pid, sigCheck.lastError().c_str());
    }

    pid_validity_cache_[pid] = result;

    return result;
#else
    (void)pid;
    return true;
#endif
}
