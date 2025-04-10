#pragma once

#include "ihelperbackend.h"
#include "utils/servicecontrolmanager.h"
#include "utils/win32handle.h"
#include <QMutex>

// Windows implementation of IHelperBackend, thread safe
class HelperBackend_win : public IHelperBackend
{
    Q_OBJECT
public:
    explicit HelperBackend_win(QObject *parent = NULL);
    ~HelperBackend_win() override;

    void startInstallHelper() override;
    State currentState() const override;
    bool reinstallHelper() override;

    std::string sendCmd(int cmdId, const std::string &data) override;

protected:
    void run() override;

private:
    enum {MAX_WAIT_TIME_FOR_PIPE = 10000};
    State curState_;
    wsl::ServiceControlManager scm_;
    wsl::Win32Handle helperPipe_;
    mutable QMutex mutex_;


    void initVariables();
    bool readAllFromPipe(HANDLE hPipe, char *buf, DWORD len);
    bool writeAllToPipe(HANDLE hPipe, const char *buf, DWORD len);
    bool connectToHelper();
};
