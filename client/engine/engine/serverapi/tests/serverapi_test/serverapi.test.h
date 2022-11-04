#pragma once

#include <QList>
#include <QObject>
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/serverapi/serverapi.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/failover/ifailover.h"

class ConnectStateController_moc : public IConnectStateController
{
    Q_OBJECT
public:
    explicit ConnectStateController_moc(QObject *parent) : IConnectStateController(parent),
        state_(CONNECT_STATE_DISCONNECTED),
        prevState_(CONNECT_STATE_DISCONNECTED)
    {}

    CONNECT_STATE currentState() override {  return state_;  }
    CONNECT_STATE prevState() override  {  return prevState_; }

    DISCONNECT_REASON disconnectReason() override
    {
        return DISCONNECTED_ITSELF;
    }
    CONNECT_ERROR connectionError() override  { return NO_CONNECT_ERROR;  }
    const LocationID& locationId() override  { return lid_;  }
    void setState(CONNECT_STATE state)
    {
        if (state != state_) {
            prevState_ = state_;
            state_ = state;
            emit stateChanged(state_, DISCONNECTED_ITSELF, NO_CONNECT_ERROR, lid_);
        }
    }

private:
    LocationID lid_;
    CONNECT_STATE state_;
    CONNECT_STATE prevState_;
};

class NetworkDetectionManager_moc : public INetworkDetectionManager
{
    Q_OBJECT
public:
    NetworkDetectionManager_moc(QObject *parent) : INetworkDetectionManager(parent) {}
    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface) override {};
    bool isOnline() override { return bOnline_; }
    void setOnline( bool bOnline)
    {
        if (bOnline != bOnline_) {
            bOnline_ = bOnline;
            emit onlineStateChanged(bOnline_);
        }
    }

private:
    bool bOnline_ = true;
};

class Failover_moc : public failover::IFailover
{
    Q_OBJECT

public:
    explicit Failover_moc(QObject *parent, QVector<QPair<QString, failover::FailoverRetCode> > failovers) : IFailover(parent),
        failovers_(failovers), curFailover_(0)
    {}

    void reset() override
    {
        curFailover_ = 0;
    }
    void getNextHostname(bool bIgnoreSslErrors) override
    {
        QTimer::singleShot(0, this, [this] () {
            emit nextHostnameAnswer(failovers_[curFailover_].second, failovers_[curFailover_].first);
        });
        curFailover_++;
    }
    void setApiResolutionSettings(const types::ApiResolutionSettings &apiResolutionSettings) override
    {
    }

private:
    QVector<QPair<QString, failover::FailoverRetCode> > failovers_;
    int curFailover_;
};

// Semi-automatic unit test of ServerAPI,
// some data needs to be entered before testing: authHash, username, password
class ServerApi_test : public QObject
{
    Q_OBJECT

public:
    ServerApi_test();

private slots:
    void init();
    void cleanup();

    void testRequests();
    void testFailoverFailed();
    void testFailoverSslError();
    void testDeleteServerAPIWhileRequestsRunning();
    void testDeleteRequestsBeforeFinished();

private:
    ConnectStateController_moc *connectStateController_;
    NetworkDetectionManager_moc *networkDetectionManager_;
    NetworkAccessManager *accessManager_;
    server_api::ServerAPI *serverAPI_;
    QString authHash_;
    QString username_;
    QString password_;
};





