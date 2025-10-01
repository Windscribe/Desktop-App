#pragma once

#include <QFile>
#include <QMutex>
#include <QLoggingCategory>
#include <spdlog/spdlog.h>

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

    // To use spdlog instead of qDebug
    spdlog::logger *getSpdLogger(const std::string &category);

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
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger> > spd_loggers_;
};

}  // namespace log_utils
