#pragma once

#include "crashhandler.h"
#include <memory>
#include <string>

#if defined(ENABLE_CRASH_REPORTS)

namespace Debug {

class CrashDumpInternal;

class CrashDump final
{
public:
    CrashDump();
    ~CrashDump();

    bool writeToFile(const std::wstring &filename, unsigned int thread_handle,
                     void *exception_pointers);

private:
    std::unique_ptr<CrashDumpInternal> internal_;
};

}

#endif  // ENABLE_CRASH_REPORTS
