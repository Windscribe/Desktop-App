#include "OpenFileLimit.h"

#include <algorithm>
#include <sys/sysctl.h>

bool raiseOpenFileLimit(rlim_t target, rlim_t &result)
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
        return false;
    }

    rlim_t cap = std::min(target, limit.rlim_max);
    int maxFilesPerProc = 0;
    size_t size = sizeof(maxFilesPerProc);
    if (sysctlbyname("kern.maxfilesperproc", &maxFilesPerProc, &size, nullptr, 0) == 0 && maxFilesPerProc > 0) {
        cap = std::min(cap, static_cast<rlim_t>(maxFilesPerProc));
    }

    if (cap > limit.rlim_cur) {
        limit.rlim_cur = cap;
        if (setrlimit(RLIMIT_NOFILE, &limit) != 0) {
            return false;
        }
    }

    result = limit.rlim_cur;
    return true;
}
