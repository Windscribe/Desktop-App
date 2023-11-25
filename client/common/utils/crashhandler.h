#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <string>

// Currently, crash reporting/minidump creation functionality is Windows-only.
#if defined(_WIN32)
#define ENABLE_CRASH_REPORTS
#endif  // _WIN32

#if defined(WINDSCRIBE_SERVICE)
#define CRASH_LOG(...) Logger::instance().out(__VA_ARGS__);
#define CRASH_ASSERT(x) assert((x))
#else
#define CRASH_LOG(...) qCDebug(LOG_BASIC, __VA_ARGS__)
#define CRASH_ASSERT(x) Q_ASSERT(x)
#endif

#if defined(ENABLE_CRASH_REPORTS)

namespace Debug {

struct ProcessExceptionHandlers;
struct ThreadExceptionHandlers;
struct ExceptionInfo;

class CrashHandler final
{
public:
    static void setModuleName(const std::wstring &moduleName) { moduleName_ = moduleName; }
    static CrashHandler& instance() { static CrashHandler local_instance; return local_instance; }

    ~CrashHandler();
    bool bindToProcess();
    void unbindFromProcess();
    bool bindToThread();
    void unbindFromThread();
    void handleException(ExceptionInfo info);

    void crashForTest() const;
    void stackOverflowForTest() const;

private:
    CrashHandler();

    void getExceptionPointers(void *exceptionPointers) const;
    std::wstring getCrashDumpFilename() const;

    static std::wstring moduleName_;
    std::mutex crashMutex_;
    std::mutex threadHandlerMutex_;
    std::unique_ptr<ProcessExceptionHandlers> oldProcessHandlers_;
    std::map<unsigned int,std::unique_ptr<ThreadExceptionHandlers>> oldThreadHandlers_;
};

class CrashHandlerForThread final
{
public:
    CrashHandlerForThread()
    {
        bind_status_ = CrashHandler::instance().bindToThread();
    }
    ~CrashHandlerForThread()
    {
        if (bind_status_)
            CrashHandler::instance().unbindFromThread();
    }
private:
    bool bind_status_ = false;
};

}

#define BIND_CRASH_HANDLER_FOR_THREAD() \
    Debug::CrashHandlerForThread bind_crash_handler_to_this_thread; \
    (void)bind_crash_handler_to_this_thread;

#else  // ENABLE_CRASH_REPORTS

#define BIND_CRASH_HANDLER_FOR_THREAD() /* */

#endif  // ENABLE_CRASH_REPORTS
