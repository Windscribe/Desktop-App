#ifndef CRASHDUMP_H
#define CRASHDUMP_H

#include "crashhandler.h"
#include <memory>

#if defined(ENABLE_CRASH_REPORTS)
#include <QString>

namespace Debug {

class CrashDumpInternal;

class CrashDump final
{
public:
    CrashDump();
    ~CrashDump();

    bool writeToFile(const QString &filename, Qt::HANDLE thread_handle, void *exception_pointers);

private:
    std::unique_ptr<CrashDumpInternal> internal_;
};

}

#endif  // ENABLE_CRASH_REPORTS
#endif // CRASHDUMP_H
