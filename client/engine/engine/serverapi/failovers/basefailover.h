#pragma once

#include <QDebug>
#include <QObject>

class NetworkAccessManager;

namespace server_api {

enum class FailoverRetCode { kSuccess, kSslError, kFailed, kConnectStateChanged };

class BaseFailover : public QObject
{
    Q_OBJECT
public:
    explicit BaseFailover(QObject *parent, NetworkAccessManager *networkAccessManager = nullptr) : QObject(parent), networkAccessManager_(networkAccessManager) {}
    virtual void getHostnames(bool bIgnoreSslErrors) = 0;
    virtual QString name() const = 0;

signals:
    void finished(server_api::FailoverRetCode retCode, const QStringList &hostnames);

protected:
     NetworkAccessManager *networkAccessManager_;
};

QDebug operator<<(QDebug dbg, const FailoverRetCode &f);


} // namespace server_api

