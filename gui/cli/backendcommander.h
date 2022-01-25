#ifndef BACKENDCOMMANDER_H
#define BACKENDCOMMANDER_H

#include <QElapsedTimer>
#include <QObject>
#include "cliapplication.h"
#include "ipc/iconnection.h"
#include "ipc/command.h"

class BackendCommander : public QObject
{
    Q_OBJECT
public:
    BackendCommander(CliCommand cmd, const QString &location);
    ~BackendCommander();

    void initAndSend();

signals:
    void finished(const QString &errorMsg);
    void report(const QString &msg);

private slots:
    void onConnectionNewCommand(IPC::Command *command, IPC::IConnection *connection);
    void onConnectionStateChanged(int state, IPC::IConnection *connection);
    void onConnectionConnectAttempt();

private:
    enum IPC_STATE { IPC_INIT_STATE, IPC_CONNECTING, IPC_CONNECTED };
    IPC_STATE ipcState_;

    static constexpr int MAX_WAIT_TIME_MS = 10000;   // 10 sec - maximum waiting time for connection to the GUI
    IPC::IConnection *connection_;
    QElapsedTimer connectingTimer_;
    CliCommand command_;
    QString locationStr_;

    void sendOneCommand();
    void sendCommand();

    bool receivedStateInit_;
    bool receivedLocationsInit_;
    bool sent_;
};

#endif // BACKENDCOMMANDER_H
