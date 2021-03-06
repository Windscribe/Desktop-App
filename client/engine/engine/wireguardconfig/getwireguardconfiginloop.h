#ifndef GETWIREGUARDCONFIGINLOOP_H
#define GETWIREGUARDCONFIGINLOOP_H

#include "getwireguardconfig.h"

#include <QTimer>

// It's actually a wrapper for GetWireGuardConfig, which tries to make requests in a loop until the first success request.
// It's used in the class ConnectionManager
// Moved to a separate class for the reason that not to overcomplicate the logic in GetWireGuardConfig or ConnectionManager classes.

class GetWireGuardConfigInLoop : public QObject
{
    Q_OBJECT
public:
    GetWireGuardConfigInLoop(QObject *parent, ServerAPI *serverAPI, uint serverApiUserRole);

    void getWireGuardConfig(const QString &serverName, bool deleteOldestKey);
    void stop();

signals:
    void getWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, const WireGuardConfig &config);

private slots:
    void onGetWireGuardConfigAnswer(SERVER_API_RET_CODE retCode, const WireGuardConfig &config);
    void onFetchWireGuardConfig();

private:
    enum {LOOP_PERIOD = 2000}; // 2 sec
    ServerAPI *serverAPI_;
    uint serverApiUserRole_;
    GetWireGuardConfig *getConfig_;
    QTimer *fetchWireguardConfigTimer_;
    QString serverName_;
    bool deleteOldestKey_;
};

#endif // GETWIREGUARDCONFIGINLOOP_H
