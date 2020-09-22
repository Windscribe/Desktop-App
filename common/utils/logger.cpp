#include "logger.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

QFile *Logger::file_ = NULL;
QMutex Logger::mutex_;
QString Logger::strLog_;
QString Logger::logPath_;
QString Logger::prevLogPath_;
bool Logger::consoleOutput_;
QtMessageHandler Logger::prevMessageHandler_ = NULL;

Q_LOGGING_CATEGORY(LOG_BASIC, "basic")
Q_LOGGING_CATEGORY(LOG_IPC, "ipc")
Q_LOGGING_CATEGORY(LOG_CONNECTION, "connection")
Q_LOGGING_CATEGORY(LOG_SERVER_API, "server_api")
Q_LOGGING_CATEGORY(LOG_CURL_MANAGER, "curl_manager")
Q_LOGGING_CATEGORY(LOG_PING, "ping ")
Q_LOGGING_CATEGORY(LOG_OPENVPN, "openvpn")
Q_LOGGING_CATEGORY(LOG_IKEV2, "ikev2")
Q_LOGGING_CATEGORY(LOG_LOCAL_WEBSERVER, "local_web")
Q_LOGGING_CATEGORY(LOG_SOCKS_SERVER, "socks_server")
Q_LOGGING_CATEGORY(LOG_HTTP_SERVER, "http_server")
Q_LOGGING_CATEGORY(LOG_WLAN_MANAGER, "wlan_manager")
Q_LOGGING_CATEGORY(LOG_NETWORK_EXTENSION_MAC, "network_extension")
Q_LOGGING_CATEGORY(LOG_EMERGENCY_CONNECT, "emergency_connect")
Q_LOGGING_CATEGORY(LOG_FIREWALL_CONTROLLER, "firewall_controller")
Q_LOGGING_CATEGORY(LOG_BEST_LOCATION, "best_location")
Q_LOGGING_CATEGORY(LOG_WSTUNNEL, "wstunnel")
Q_LOGGING_CATEGORY(LOG_CUSTOM_OVPN, "custom_ovpn")
Q_LOGGING_CATEGORY(LOG_WIREGUARD, "wireguard")

Q_LOGGING_CATEGORY(LOG_USER,  "gui")

void Logger::install(const QString &name, bool consoleOutput)
{
    //QLoggingCategory::setFilterRules("basic=false\nipc=false\nserver_api=false");

    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QDir dir(logFilePath);
    dir.mkpath(logFilePath);
    prevLogPath_ = logFilePath + "/prev_log_" + name + ".txt";
    logFilePath += "/log_" + name + ".txt";
    logPath_ = logFilePath;
    copyToPrevLog();
    QMutexLocker lock(&mutex_);
    file_ = new QFile(logFilePath);
    file_->open(QIODevice::WriteOnly);

    consoleOutput_ = consoleOutput;


    prevMessageHandler_ = qInstallMessageHandler(myMessageHandler);
}

Logger::Logger()
{
}

Logger::~Logger()
{
    QMutexLocker lock(&mutex_);
    if (file_)
    {
        file_->close();
        delete file_;
    }
}

void Logger::copyToPrevLog()
{
    if (QFile::exists(logPath_))
    {
        QFile::remove(prevLogPath_);
        QFile::copy(logPath_, prevLogPath_);
    }
}

void Logger::myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &s)
{
    {
        QMutexLocker lock(&mutex_);

        if (file_)
        {
            QString str = qFormatLogMessage(type, context, s);
            QString strDateTime = QDateTime::currentDateTimeUtc().toString("ddMM hh:mm:ss:zzz");
            str.replace("{gmt_time}", strDateTime);

            file_->write(str.toLocal8Bit());
            file_->write("\r\n");
            file_->flush();
            strLog_ += str + "\n";
        }
    }
    if (consoleOutput_)
    {
        prevMessageHandler_(type, context, s);
    }
}

QString Logger::getLogStr()
{
    QMutexLocker lock(&mutex_);
    QString ret;
    QFile prevFileLog(prevLogPath_);
    if (prevFileLog.open(QIODevice::ReadOnly))
    {
        ret = prevFileLog.readAll();
        ret += "----------------------------------------------------------------\n";
        prevFileLog.close();
    }
    ret += strLog_;
    return ret;
}

QString Logger::getCurrentLogStr()
{
    QMutexLocker lock(&mutex_);
    return strLog_;
}
