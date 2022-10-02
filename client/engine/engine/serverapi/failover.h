#pragma once

#include "failovers/basefailover.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"

namespace server_api {

// The failover algorithm. Allows us to switch sequentially to the next domain and hides implementation details.
// more details: https://hub.int.windscribe.com/en/Infrastructure/References/Client-Control-Plane-Endpoint-Failover
// Some failover algorithms require IConnectStateController, in order to ensure the atomicity of the execution of a network request
// in the connected state or disconnected VPN state.In other words, a network request that failed when switching the VPN, for example
// from the connected state to the disconnected state, is not considered a failed attempt.
class Failover : public QObject
{
    Q_OBJECT
public:
    explicit Failover(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController);

    // can return an empty string if the Failover in the FailoverRetCode::kFailed state
    QString currentHostname() const;
    void reset();
    void getNextHostname(bool bIgnoreSslErrors);

signals:
    void nextHostnameAnswer(server_api::FailoverRetCode retCode, const QString &hostname);

private slots:
    void onFailoverFinished(server_api::FailoverRetCode retCode, const QStringList &hostnames);

private:
    QVector<BaseFailover *> failovers_;
    int curFailoverInd_ = -1;
    QStringList curFailoverHostnames_;
    int cutFaiolverHostnameInd_ = -1;
    bool isFailoverInProgress_ = false;
    bool bIgnoreSslErrors_ = false;

    QStringList randomizeList(const QStringList &list);
};

} // namespace server_api
