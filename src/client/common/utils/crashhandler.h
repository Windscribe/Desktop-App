#pragma once

#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/xchar.h>

// Currently, crash reporting/minidump creation functionality is Windows-only.
#if defined(_WIN32)
#define ENABLE_CRASH_REPORTS
#endif  // _WIN32

#if !defined(WINDSCRIBE_SERVICE)
    #include "utils/log/categories.h"
#endif

template <typename T, class ...Args>
void CRASH_LOG_ERROR(const T *str, Args &&... args)
{
#if defined(WINDSCRIBE_SERVICE)
    spdlog::error(str, args...);
#else
    auto s = fmt::format(str, args...);
    if constexpr(std::is_same<char, T>::value)
        qCCritical(LOG_BASIC) << QString::fromStdString(s);
    else if constexpr(std::is_same<wchar_t, T>::value)
        qCCritical(LOG_BASIC) << QString::fromStdWString(s);
#endif
}

template <typename T, class ...Args>
void CRASH_LOG_INFO(const T *str, Args &&... args)
{
#if defined(WINDSCRIBE_SERVICE)
    spdlog::info(str, args...);
#else
    auto s = fmt::format(str, args...);
    if constexpr(std::is_same<char, T>::value)
        qCInfo(LOG_BASIC) << QString::fromStdString(s);
    else if constexpr(std::is_same<wchar_t, T>::value)
        qCInfo(LOG_BASIC) << QString::fromStdWString(s);
#endif
}

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
