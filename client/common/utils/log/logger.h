#pragma once

#include <QFile>
#include <QMutex>
#include <QLoggingCategory>

namespace log_utils {

class Logger
{
public:
    static Logger &instance()
    {
        static Logger l;
        return l;
    }

    bool install(const QString &logFilePath, bool consoleOutput);

    void setConsoleOutput(bool on);

    void startConnectionMode(const std::string &id);
    void endConnectionMode();
    const QLoggingCategory& connectionModeLoggingCategory();

private:
    Logger();

    static void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &s);

private:
    QMutex mutex_;
    QtMessageHandler prevMessageHandler_ = nullptr;

    // #23. If logger is in connection mode then it will use CONNECTION_MODE logging category for each log line.
    // It will use randomly generated sequence as current connection identifier.
    // Usually it occurs between connection start and next disconnect event.
    bool connectionMode_;
    std::unique_ptr<QLoggingCategory> connectionCategoryDefault_;
    std::unique_ptr<QLoggingCategory> connectionModeLoggingCategory_;
};

}  // namespace log_utils
