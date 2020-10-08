#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QMutex>
#include <QLoggingCategory>

// log categories
Q_DECLARE_LOGGING_CATEGORY(LOG_BASIC)
Q_DECLARE_LOGGING_CATEGORY(LOG_IPC)
Q_DECLARE_LOGGING_CATEGORY(LOG_CONNECTION)
Q_DECLARE_LOGGING_CATEGORY(LOG_SERVER_API)
Q_DECLARE_LOGGING_CATEGORY(LOG_CURL_MANAGER)
Q_DECLARE_LOGGING_CATEGORY(LOG_PING)
Q_DECLARE_LOGGING_CATEGORY(LOG_OPENVPN)
Q_DECLARE_LOGGING_CATEGORY(LOG_IKEV2)
Q_DECLARE_LOGGING_CATEGORY(LOG_LOCAL_WEBSERVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_SOCKS_SERVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_HTTP_SERVER)
Q_DECLARE_LOGGING_CATEGORY(LOG_WLAN_MANAGER)
Q_DECLARE_LOGGING_CATEGORY(LOG_NETWORK_EXTENSION_MAC)
Q_DECLARE_LOGGING_CATEGORY(LOG_EMERGENCY_CONNECT)
Q_DECLARE_LOGGING_CATEGORY(LOG_FIREWALL_CONTROLLER)
Q_DECLARE_LOGGING_CATEGORY(LOG_BEST_LOCATION)
Q_DECLARE_LOGGING_CATEGORY(LOG_WSTUNNEL)
Q_DECLARE_LOGGING_CATEGORY(LOG_CUSTOM_OVPN)
Q_DECLARE_LOGGING_CATEGORY(LOG_DOWNLOADER)
Q_DECLARE_LOGGING_CATEGORY(LOG_VOLUME_HELPER)

// for GUI
Q_DECLARE_LOGGING_CATEGORY(LOG_USER)


class Logger
{
public:
    static Logger &instance()
    {
        static Logger l;
        return l;
    }

    void install(const QString &name, bool consoleOutput);
    void setConsoleOutput(bool on);
    QString getLogStr();
    QString getCurrentLogStr();

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

    static void copyToPrevLog();
};


#endif // LOGGER_H
