#pragma once

#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "failover.h"
#include "requests/baserequest.h"

namespace server_api {

enum class FailoverDetectionRetCode { kSuccess, kSslError, kConnectStateChanged };

// Helper class for ServerAPI, applies the Failover algorithm sequentially to the request until success or openssl error.
// It also depends on the VPN connection status. If the state has changed (disconnected -> connected or connected -> disconnected) during the detection, it ends with an error.
class FailoverDetection : public QObject
{
    Q_OBJECT
public:
    explicit FailoverDetection(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController, BaseRequest *request, Failover *failover);

    void start();
    BaseRequest *request() const { return request_; }

signals:
    void finished(server_api::FailoverDetectionRetCode retCode);

private slots:

private:
    BaseRequest *request_;
};

} // namespace server_api
