#include "logger.h"


#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QString>

QFile *Logger::file_ = NULL;
QMutex Logger::mutex_;
QString Logger::strLog_;
QString Logger::logPath_;
QString Logger::prevLogPath_;
bool Logger::consoleOutput_;
QtMessageHandler Logger::prevMessageHandler_ = NULL;
bool Logger::connectionMode_ = false;
QLoggingCategory* Logger::connectionModeLoggingCategory_ = nullptr;

#define WS_LOGGING_CATEGORY(name, ...) \
const QLoggingCategory &name() \
    { \
        QMutexLocker lock(&Logger::mutex()); \
        if (!Logger::instance().connectionMode()) {\
                static const QLoggingCategory category(__VA_ARGS__); \
                return category; \
        }\
        else { \
            return Logger::instance().connectionModeLoggingCategory(); \
        } \
    }

Q_LOGGING_CATEGORY(LOG_BASIC, "basic")
Q_LOGGING_CATEGORY(LOG_IPC, "ipc")
Q_LOGGING_CATEGORY(LOG_CLI_IPC, "cli_ipc")
WS_LOGGING_CATEGORY(LOG_CONNECTION, "connection")
Q_LOGGING_CATEGORY(LOG_WSNET, "wsnet")
Q_LOGGING_CATEGORY(LOG_FAILOVER, "failover")
Q_LOGGING_CATEGORY(LOG_NETWORK, "network")
Q_LOGGING_CATEGORY(LOG_PING, "ping ")
Q_LOGGING_CATEGORY(LOG_OPENVPN, "openvpn")
Q_LOGGING_CATEGORY(LOG_IKEV2, "ikev2")
Q_LOGGING_CATEGORY(LOG_LOCAL_WEBSERVER, "local_web")
Q_LOGGING_CATEGORY(LOG_SOCKS_SERVER, "socks_server")
Q_LOGGING_CATEGORY(LOG_HTTP_SERVER, "http_server")
Q_LOGGING_CATEGORY(LOG_WIFI_SHARED, "wifi_shared")
Q_LOGGING_CATEGORY(LOG_NETWORK_EXTENSION_MAC, "network_extension")
Q_LOGGING_CATEGORY(LOG_EMERGENCY_CONNECT, "emergency_connect")
Q_LOGGING_CATEGORY(LOG_FIREWALL_CONTROLLER, "firewall_controller")
Q_LOGGING_CATEGORY(LOG_BEST_LOCATION, "best_location")
Q_LOGGING_CATEGORY(LOG_WSTUNNEL, "wstunnel")
Q_LOGGING_CATEGORY(LOG_CTRLD, "ctrld")
Q_LOGGING_CATEGORY(LOG_CUSTOM_OVPN, "custom_ovpn")
Q_LOGGING_CATEGORY(LOG_WIREGUARD, "wireguard")
Q_LOGGING_CATEGORY(LOG_PACKET_SIZE, "packet_size")
Q_LOGGING_CATEGORY(LOG_DOWNLOADER, "downloader")
Q_LOGGING_CATEGORY(LOG_AUTO_UPDATER, "auto_updater")
Q_LOGGING_CATEGORY(LOG_LAUNCH_ON_STARTUP, "launch_on_startup")
Q_LOGGING_CATEGORY(LOG_CONNECTED_DNS, "connected_dns")
Q_LOGGING_CATEGORY(LOG_AUTH_HELPER, "auth_helper")
Q_LOGGING_CATEGORY(LOG_ASSERT, "assert")


Q_LOGGING_CATEGORY(LOG_USER,  "user")
Q_LOGGING_CATEGORY(LOG_LOCATION_LIST, "loclist");
Q_LOGGING_CATEGORY(LOG_PREFERENCES, "prefs")


void Logger::install(const QString &name, bool consoleOutput, bool recoveryMode)
{
    QLoggingCategory::setFilterRules("qt.tlsbackend.ossl=false\nqt.network.ssl=false");

    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(logFilePath);
    dir.mkpath(logFilePath);
    prevLogPath_ = logFilePath + "/prev_log_" + name + ".txt";
    logFilePath += "/log_" + name + ".txt";
    logPath_ = logFilePath;

    // in recovery mode do not move the log_engine to be prev_log_engine to preserve the potential crash log
    // recovery mode only relevant for engine
    QIODevice::OpenModeFlag openModeFlag = QIODevice::Append;
    if (!recoveryMode)
    {
        copyToPrevLog();
        openModeFlag = QIODevice::WriteOnly;
    }

    QMutexLocker lock(&mutex_);
    file_ = new QFile(logFilePath);
    file_->open(openModeFlag);
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

    if (connectionModeLoggingCategory_)
        delete connectionModeLoggingCategory_;
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
            QString strDateTime = QDateTime::currentDateTimeUtc().toString("ddMMyy hh:mm:ss:zzz");
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

void Logger::startConnectionMode()
{
    QMutexLocker lock(&mutex_);
    connectionMode_ = true;
    const qint64 currentTimeMillis = QDateTime::currentMSecsSinceEpoch();
    const QByteArray timeBytes(reinterpret_cast<const char*>(&currentTimeMillis), sizeof(currentTimeMillis));
    const QByteArray hashBytes = QCryptographicHash::hash(timeBytes, QCryptographicHash::Md5);

    static std::string connectionModeId;
    connectionModeId = hashBytes.toHex().mid(8).toStdString();
    connectionModeLoggingCategory_ = new QLoggingCategory(connectionModeId.c_str());
}

void Logger::endConnectionMode()
{
    QMutexLocker lock(&mutex_);
    connectionMode_ = false;
    if (connectionModeLoggingCategory_) {
        delete connectionModeLoggingCategory_;
        connectionModeLoggingCategory_ = nullptr;
    }
}
