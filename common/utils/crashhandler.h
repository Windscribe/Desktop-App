#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include <memory>
#include <QMap>
#include <QMutex>
#include <QString>

// Currently, crash reporting/minidump creation functionality is Windows-only.
#if defined(Q_OS_WIN)
#define ENABLE_CRASH_REPORTS
#endif  // Q_OS_WIN

namespace Debug {

#if defined(ENABLE_CRASH_REPORTS)

struct ProcessExceptionHandlers;
struct ThreadExceptionHandlers;
struct ExceptionInfo;

class CrashHandler final
{
public:
    static void setModuleName(const QString &moduleName) { moduleName_ = moduleName; }
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
    QString getCrashDumpFilename() const;

    static QString moduleName_;
    QMutex crashMutex_;
    QMutex threadHandlerMutex_;
    std::unique_ptr<ProcessExceptionHandlers> oldProcessHandlers_;
    std::map<Qt::HANDLE,std::unique_ptr<ThreadExceptionHandlers>> oldThreadHandlers_;
};

#endif  // ENABLE_CRASH_REPORTS

class CrashHandlerForThread final
{
public:
    CrashHandlerForThread()
    {
#if defined(ENABLE_CRASH_REPORTS)
        bind_status_ = CrashHandler::instance().bindToThread();
#endif  // ENABLE_CRASH_REPORTS
    }
    ~CrashHandlerForThread()
    {
#if defined(ENABLE_CRASH_REPORTS)
        if (bind_status_)
            CrashHandler::instance().unbindFromThread();
#endif  // ENABLE_CRASH_REPORTS
    }
private:
    bool bind_status_ = false;
};

}

#endif // CRASHHANDLER_H
