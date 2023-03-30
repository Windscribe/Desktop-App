#pragma once

#include <QElapsedTimer>
#include <QObject>

#include "cliarguments.h"
#include "ipc/command.h"
#include "ipc/iconnection.h"

class BackendCommander : public QObject
{
    Q_OBJECT
public:
    BackendCommander(const CliArguments &cliArgs);
    ~BackendCommander();

    void initAndSend(bool isGuiAlreadyRunning);

signals:
    void finished(int returnCode, const QString &errorMsg);
    void report(const QString &msg);

private slots:
    void onConnectionNewCommand(IPC::Command *command, IPC::IConnection *connection);
    void onConnectionStateChanged(int state, IPC::IConnection *connection);

    void sendCommand();
    void sendStateCommand();

private:
    const CliArguments &cliArgs_;
    enum IPC_STATE { IPC_INIT_STATE, IPC_CONNECTING, IPC_CONNECTED };
    IPC_STATE ipcState_ = IPC_INIT_STATE;

    static constexpr int MAX_WAIT_TIME_MS = 10000;   // 10 sec - maximum waiting time for connection to the GUI
    static constexpr int MAX_LOGIN_TIME_MS = 10000;   // 10 sec - maximum waiting time for login in the GUI
    IPC::IConnection *connection_ = nullptr;
    QElapsedTimer connectingTimer_;
    QElapsedTimer loggedInTimer_;
    bool bCommandSent_ = false;
    bool bLogginInMessageShown_ = false;
    bool isGuiAlreadyRunning_ = false;

    void onStateResponse(IPC::Command *command);
};
