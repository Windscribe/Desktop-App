#ifndef BACKENDCOMMANDER_H
#define BACKENDCOMMANDER_H

#include <QObject>
#include "cliapplication.h"

#include "../Backend/backend.h"

class BackendCommander : public QObject
{
    Q_OBJECT
public:
    BackendCommander(CliCommand cmd, const QString &location);
    ~BackendCommander();

    void initAndSend();
    void closeBackendConnection();

signals:
    void finished(QString errorMsg);
    void report(const QString &msg);

private slots:
    void onBackendInitFinished(ProtoTypes::InitState state);
    void onBackendFirewallStateChanged(bool isEnabled);
    void onBackendConnectStateChanged(ProtoTypes::ConnectState state);
    void onBackendLocationsUpdated();

private:
    Backend *backend_;
    CliCommand command_;
    QString locationStr_;

    void sendOneCommand();
    void sendCommand();

    bool receivedStateInit_;
    bool receivedLocationsInit_;
    bool sent_;
};

#endif // BACKENDCOMMANDER_H
