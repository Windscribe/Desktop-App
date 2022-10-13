#pragma once

#include "ifailover.h"
#include "failovers/basefailover.h"

namespace failover {

// The failover algorithm. Allows us to switch sequentially to the next domain.
// more details: https://hub.int.windscribe.com/en/Infrastructure/References/Client-Control-Plane-Endpoint-Failover
// Some failover algorithms require IConnectStateController, in order to ensure the atomicity of the execution of a network request
// in the connected state or disconnected VPN state.In other words, a network request that failed when switching the VPN, for example
// from the connected state to the disconnected state, is not considered a failed attempt.
class Failover : public IFailover
{
    Q_OBJECT
public:
    explicit Failover(QObject *parent, NetworkAccessManager *networkAccessManager, IConnectStateController *connectStateController, const QString &nameForLog);

    // can return an empty string if the Failover in the FailoverRetCode::kFailed state
    QString currentHostname() const override;
    void reset() override;  // keep in mind that this function also emits the signal onFailoverFinished()
    void getNextHostname(bool bIgnoreSslErrors) override;   // emits the signal onFailoverFinished()

    void setApiResolutionSettings(const types::ApiResolutionSettings &apiResolutionSettings) override;

private slots:
    void onFailoverFinished(FailoverRetCode retCode, const QStringList &hostnames);

private:
    QString nameForLog_;
    QVector<BaseFailover *> failovers_;
    int curFailoverInd_ = -1;
    QStringList curFailoverHostnames_;
    int curFaiolverHostnameInd_ = -1;
    bool isFailoverInProgress_ = false;
    bool bIgnoreSslErrors_ = false;
    types::ApiResolutionSettings apiResolutionSettings_;

    QStringList randomizeList(const QStringList &list);
};

} // namespace failover
