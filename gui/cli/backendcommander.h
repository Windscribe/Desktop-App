#pragma once

#include <QElapsedTimer>
#include <QObject>

#include "cliarguments.h"
#include "ipc/clicommands.h"
#include "ipc/command.h"
#include "ipc/connection.h"

class BackendCommander : public QObject
{
    Q_OBJECT
public:
    BackendCommander(const CliArguments &cliArgs);
    ~BackendCommander();

    void initAndSend();

signals:
    void finished(int returnCode, const QString &errorMsg);
    void report(const QString &msg);

private slots:
    void onConnectionNewCommand(IPC::Command *command, IPC::Connection *connection);
    void onConnectionStateChanged(int state, IPC::Connection *connection);

    void sendCommand(IPC::CliCommands::State *state);
    void sendStateCommand();

private:
    const CliArguments &cliArgs_;
    enum IPC_STATE { IPC_INIT_STATE, IPC_CONNECTING, IPC_CONNECTED };
    IPC_STATE ipcState_ = IPC_INIT_STATE;

    static constexpr int MAX_WAIT_TIME_MS = 10000;   // 10 sec - maximum waiting time for connection to the GUI
    static constexpr int MAX_LOGIN_TIME_MS = 10000;   // 10 sec - maximum waiting time for login in the GUI
    IPC::Connection *connection_ = nullptr;
    QElapsedTimer connectingTimer_;
    QElapsedTimer loggedInTimer_;
    bool bCommandSent_ = false;
    bool bLoggingInMessageShown_ = false;

    void onStateResponse(IPC::Command *command);
};
