#include <limits.h>
#include <fstream>
#include <sstream>

#include "boost/filesystem/path.hpp"

#include "helper_security.h"
#include "logger.h"

#include "../../../common/utils/executable_signature/executablesignature_linux.h"

// TODO: perhaps would be better to embed this as a resource at compile time
// in case the public key changes?
// We don't have access to the Qt resource system in the Linux helper app, so:
// - Add Qt core support to the helper so we can use the Qt resource system, or
// - See if we can use something like https://github.com/graphitemaster/incbin/

#if defined(USE_SIGNATURE_CHECK_ON_LINUX)
static const char g_PublicKeyData[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAxjYavNvrEtNV2kccwb3k\n"
    "RzTBRmBzD8TnllPo4rBH2L0XWjP9y1u3ZztTl0366uMRYLrWYIpKqCWloOfdydFa\n"
    "b672lfzJlYNCWMTzSEA/IFOaOe/Fdnf1qHlCF8u+q3BUah+Ki72kJnzLyi9DpdA6\n"
    "DHq7I/K5cXCf5lGZ95rFzblPGygCpbKf48ecPhbED5V/350OCCQ3U7HvxNrkfjxg\n"
    "CZ67ZiVo3f1MF2beQF3U33DJxa9hGDlyxdoWE7zMKzz448xVm568XnvLni9lOX/m\n"
    "6xpLrrS/zXStWOTdhtvQasdBTDYxcII3tkfK7IQIZpio8+CzzgnI5Z9Y7OIztJ4U\n"
    "sOHykACXbsLpzT+8tKzNypTZOSd8m6PGqc8eZCsx2YPLa7F23FUEW46pulzmgup7\n"
    "Gj4XYaZInlEpkmH/MUil3bUAhCIDT//7cUwQnroHCqpNLf++Epyz0bwBXiQUkVGk\n"
    "QMrafJn28Q4tmFtzMXBw0gJHxmijTq0lMnssTZMwOxdOZeKWyhSR8fQMhUGfioOc\n"
    "2QSc0coonsJ8PTFtM7mXcny5CRdo3wxYfbnze7JsOMo6BuvihbFMkGxIbaQ4X6Wn\n"
    "6qmkgVrSBw67KwxCOEfIjwNjsCyDypRLEAfIU0sVSOknKXNpPgBqRkADRCD/F14n\n"
    "36htEHoGyqRdFOaURnR9lB0CAwEAAQ==\n"
    "-----END PUBLIC KEY-----";
#endif

// Expects symLink to reference /path/*/exe, where * can be 'self', or a pid, or
// and exe name.
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
// TODO: replace with QtCoreApplication::applicationDirPath() if we enable Qt
// support in the helper somewhere down the digital road.
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

void HelperSecurity::reset()
{
    pid_validity_cache_.clear();
}

bool HelperSecurity::verifyProcessId(pid_t pid)
{
    const auto it = pid_validity_cache_.find(pid);
    if (it != pid_validity_cache_.end())
        return it->second;

    return verifyProcessIdImpl(pid);
}

bool HelperSecurity::verifyProcessIdImpl(pid_t pid)
{
    //Logger::instance().out("Getting exe path and name for PID %i", pid);

    const std::string clientAppPath = getProcessPath(pid);
    if (clientAppPath.empty())
    {
        Logger::instance().out("Failed to get exe path and name for PID %i, errno %d", pid, errno);
        pid_validity_cache_[pid] = false;
        return false;
    }

    const std::string appDirPath    = applicationDirPath();
    const std::string engineExePath = appDirPath + "/WindscribeEngine";

    //Logger::instance().out("Checking exe path matches engine's: %s", clientAppPath.c_str());

    if (engineExePath.compare(clientAppPath) != 0)
    {
        Logger::instance().out("Invalid calling application for PID %i, %s", pid, clientAppPath.c_str());
        pid_validity_cache_[pid] = false;
        return false;
    }

    #if defined(USE_SIGNATURE_CHECK_ON_LINUX)
    const std::string sigPath = appDirPath + "/signatures/WindscribeEngine.sig";

    //Logger::instance().out("Verifying signature...");

    ExecutableSignature_linux verifySignature;
    bool result = verifySignature.verifyWithPublicKey(engineExePath, sigPath, g_PublicKeyData);

    if (!result) {
        Logger::instance().out("Signature verification failed for PID %i, %s", pid, verifySignature.lastError().c_str());
    }

    pid_validity_cache_[pid] = result;

    return result;
    #else
    pid_validity_cache_[pid] = true;
    return true;
    #endif
}
