#pragma once

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

    virtual void startInstallHelper() = 0;
    virtual State currentState() const = 0;
    virtual bool reinstallHelper() = 0;

    virtual std::string sendCmd(int cmdId, const std::string &data) = 0;

signals:
    void lostConnectionToHelper();
};
