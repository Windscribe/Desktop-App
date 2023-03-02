#pragma once

#include <QObject>
#include <QPointer>
#include <QQueue>

#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/connectstatecontroller/connectstatewatcher.h"
#include "requests/baserequest.h"
#include "engine/failover/basefailover.h"


namespace server_api {

// Helper class used by ServerAPI.
// Tries to execute a request through the specified failover and returns the result of this execution.
// In short it executes the failover request first and then, if successful, the request itself

enum class RequestExecuterRetCode { kSuccess, kRequestDeleted, kFailoverFailed, kConnectStateChanged};

class RequestExecuterViaFailover : public QObject
{
    Q_OBJECT
public:
    explicit RequestExecuterViaFailover(QObject *parent, IConnectStateController *connectStateController, NetworkAccessManager *networkAccessManager);

    void execute(QPointer<BaseRequest> request, failover::BaseFailover *failover, bool bIgnoreSslErrors);
    QPointer<BaseRequest> request() { return request_; }
    failover::FailoverData failoverData() const;

signals:
    void finished(server_api::RequestExecuterRetCode retCode);

private slots:
    void onFailoverFinished(const QVector<failover::FailoverData> &data);
    void onNetworkRequestFinished();

private:
    IConnectStateController *connectStateController_;
    NetworkAccessManager *networkAccessManager_;

    QPointer<BaseRequest> request_;
    failover::BaseFailover *failover_;
    bool bIgnoreSslErrors_;

    QScopedPointer<ConnectStateWatcher> connectStateWatcher_;

    QVector<failover::FailoverData> failoverData_;
    int curIndFailoverData_;

    void executeBaseRequest(const failover::FailoverData &failoverData);
};

} // namespace server_api
