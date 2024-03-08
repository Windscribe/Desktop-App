#pragma once

#include <QFile>
#include <QMutex>
#include <QLoggingCategory>

#include "clean_sensitive_info.h"
#include "multiline_message_logger.h"

// log categories
Q_DECLARE_LOGGING_CATEGORY(LOG_BASIC)
Q_DECLARE_LOGGING_CATEGORY(LOG_CLI_IPC)
Q_DECLARE_LOGGING_CATEGORY(LOG_IPC)
Q_DECLARE_LOGGING_CATEGORY(LOG_CONNECTION)
Q_DECLARE_LOGGING_CATEGORY(LOG_WSNET)
Q_DECLARE_LOGGING_CATEGORY(LOG_FAILOVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_NETWORK)
Q_DECLARE_LOGGING_CATEGORY(LOG_PING)
Q_DECLARE_LOGGING_CATEGORY(LOG_OPENVPN)
Q_DECLARE_LOGGING_CATEGORY(LOG_IKEV2)
Q_DECLARE_LOGGING_CATEGORY(LOG_LOCAL_WEBSERVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_SOCKS_SERVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_HTTP_SERVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_WIFI_SHARED)
Q_DECLARE_LOGGING_CATEGORY(LOG_NETWORK_EXTENSION_MAC)
Q_DECLARE_LOGGING_CATEGORY(LOG_EMERGENCY_CONNECT)
Q_DECLARE_LOGGING_CATEGORY(LOG_FIREWALL_CONTROLLER)
Q_DECLARE_LOGGING_CATEGORY(LOG_BEST_LOCATION)
Q_DECLARE_LOGGING_CATEGORY(LOG_WSTUNNEL)
Q_DECLARE_LOGGING_CATEGORY(LOG_CTRLD)
Q_DECLARE_LOGGING_CATEGORY(LOG_CUSTOM_OVPN)
Q_DECLARE_LOGGING_CATEGORY(LOG_WIREGUARD)
Q_DECLARE_LOGGING_CATEGORY(LOG_PACKET_SIZE)
Q_DECLARE_LOGGING_CATEGORY(LOG_DOWNLOADER)
Q_DECLARE_LOGGING_CATEGORY(LOG_AUTO_UPDATER)
Q_DECLARE_LOGGING_CATEGORY(LOG_LAUNCH_ON_STARTUP)
Q_DECLARE_LOGGING_CATEGORY(LOG_CONNECTED_DNS)
Q_DECLARE_LOGGING_CATEGORY(LOG_AUTH_HELPER)
Q_DECLARE_LOGGING_CATEGORY(LOG_ASSERT)


// for GUI
Q_DECLARE_LOGGING_CATEGORY(LOG_USER)
Q_DECLARE_LOGGING_CATEGORY(LOG_LOCATION_LIST)
Q_DECLARE_LOGGING_CATEGORY(LOG_PREFERENCES)


class Logger
{
public:
    static Logger &instance()
    {
        static Logger l;
        return l;
    }

    void install(const QString &name, bool consoleOutput, bool recoveryMode);
    void setConsoleOutput(bool on);
    QString getLogStr();
    QString getCurrentLogStr();

    void startConnectionMode();
    void endConnectionMode();
    static bool connectionMode() { return connectionMode_; }
    static const QLoggingCategory& connectionModeLoggingCategory() { return *connectionModeLoggingCategory_;}
    static QMutex& mutex() { return mutex_; }

private:
    Logger();
    ~Logger();

    static void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &s);

private:
    static QtMessageHandler prevMessageHandler_;

    static QFile *file_;
    static QMutex mutex_;
    static QString strLog_;
    static QString logPath_;
    static QString prevLogPath_;
    static bool consoleOutput_;

    // #23. If logger is in connection mode then it will use CONNECTION_MODE logging category for each log line.
    // It will use randomly generated sequence as current connection identifier.
    // Usually it occurs between connection start and next disconnect event.
    static bool connectionMode_;
    static QLoggingCategory *connectionModeLoggingCategory_;

    static void copyToPrevLog();
};
