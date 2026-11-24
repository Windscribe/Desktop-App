#pragma once

#include <optional>
#include <string>
#include <utility>

#include <QThread>

// Basic backend interface of the helper
// Allows to connect to the helper, track its status, send root commands, and receive a response
class IHelperBackend : public QThread
{
    Q_OBJECT
public:
    enum class State { kInit, kConnected, kFailedConnect, kUserCanceled, kInstallFailed };

    explicit IHelperBackend(QObject *parent = 0) : QThread(parent) {}
    virtual ~IHelperBackend() {}

    virtual void startInstallHelper(bool bForceDeleteOld = false) = 0;
    virtual State currentState() const = 0;
    virtual bool reinstallHelper() = 0;

    virtual std::string sendCmd(int cmdId, const std::string &data) = 0;

    // Sent ahead of any other command on each new connection to the helper. Set before startInstallHelper().
    void setConnectCommand(int cmdId, const std::string &data) { connectCommand_ = std::make_pair(cmdId, data); }

signals:
    void lostConnectionToHelper();

protected:
    std::optional<std::pair<int, std::string>> connectCommand_;
};
