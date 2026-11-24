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
    void onStateUpdated(IPC::Command *command);

    void sendCommand(IPC::CliCommands::State *state);
    void sendStateCommand();

private:
    const CliArguments &cliArgs_;
    enum IPC_STATE { IPC_INIT_STATE, IPC_CONNECTING, IPC_CONNECTED };
    IPC_STATE ipcState_ = IPC_INIT_STATE;

    static constexpr int MAX_WAIT_TIME_MS = 10000;   // 10 sec - maximum waiting time for connection to the GUI
    static constexpr int MAX_LOGIN_TIME_MS = 10000;   // 10 sec - maximum waiting time for login in the GUI
#ifndef CLI_ONLY
    static constexpr int MAX_GUI_PROMPT_TIME_MS = 120000;   // 2 min - maximum waiting time for the user to answer a GUI prompt
#endif
    IPC::Connection *connection_ = nullptr;
    QElapsedTimer connectingTimer_;
    QElapsedTimer loggedInTimer_;
#ifndef CLI_ONLY
    QElapsedTimer guiPromptTimer_;   // Runs while a prompt that only the GUI can answer is pending.
#endif
    bool bCommandSent_ = false;
    bool isUpdateInstallerRunning_ = false;
    bool bLoggingInMessageShown_ = false;
    QString connectId_ = "";
    bool isConnectRequestCountKnown_ = false;
    uint connectRequestCount_ = 0;
    QString username_;
    QString password_;
    QString code2fa_ = "";
    QString captchaSolution_;

    void onStateResponse(IPC::Command *command);
    void onUpdateStateResponse(IPC::Command *command);
    void onAcknowledge(IPC::CliCommands::Acknowledge *ackCmd);
#ifndef CLI_ONLY
    bool isLoginRequired() const;
    // Tells the user once that the GUI is waiting on them, and gives up if they do not answer in time.
    // Returns true when it has given up, i.e. emitted finished().
    bool checkGuiPrompt(bool isPending, const QString &hint, const QString &timeoutMsg);
#endif
};
