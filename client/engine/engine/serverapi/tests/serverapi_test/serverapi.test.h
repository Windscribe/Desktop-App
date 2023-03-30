#pragma once

#include <QList>
#include <QObject>
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/serverapi/serverapi.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/failover/ifailovercontainer.h"

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

class Failover_moc : public failover::BaseFailover
{
    Q_OBJECT
public:
    explicit Failover_moc(QObject *parent, QString domain, bool bSuccess) : BaseFailover(parent, "id"),  domain_(domain), bSuccess_(bSuccess) {}

    void getData(bool bIgnoreSslErrors) {
        if (bSuccess_) {
            emit finished(QVector<failover::FailoverData>() << failover::FailoverData(domain_));
        } else {
            emit finished(QVector<failover::FailoverData>());
        }
    }

    QString name() const override {
        return "failoverName: " + domain_;
    }

private:
    QString domain_;
    bool bSuccess_;
};

class FailoverContainer_moc : public failover::IFailoverContainer
{
    Q_OBJECT

public:
    explicit FailoverContainer_moc(QObject *parent, QVector<QPair<QString, bool> > failovers) : IFailoverContainer(parent),
        failovers_(failovers), curFailoverInd_(0)
    {
        resetImpl();
    }

    void reset() override
    {
        resetImpl();
    }

    QSharedPointer<failover::BaseFailover> currentFailover(int *outInd = nullptr) override
    {
        WS_ASSERT(curFailoverInd_ >= 0 && curFailoverInd_ < failovers_.size());
        if (outInd)
            *outInd = curFailoverInd_;
        return currentFailover_;
    }

    bool gotoNext() override
    {
        if (curFailoverInd_ <  (failovers_.size() - 1)) {
            curFailoverInd_++;
            currentFailover_ = QSharedPointer<failover::BaseFailover>(new Failover_moc(this, failovers_[curFailoverInd_].first, failovers_[curFailoverInd_].second));
            return !currentFailover_.isNull();
        } else {
            return false;
        }
    }

    QSharedPointer<failover::BaseFailover> failoverById(const QString &failoverUniqueId) override
    {
        return nullptr;
    }

    int count() const override
    {
        return failovers_.count();
    }

private:
    QVector<QPair<QString, bool> > failovers_;
    int curFailoverInd_;
    QSharedPointer<failover::BaseFailover> currentFailover_;

    void resetImpl()
    {
        curFailoverInd_ = 0;
        currentFailover_ = QSharedPointer<failover::BaseFailover>(new Failover_moc(this, failovers_[curFailoverInd_].first, failovers_[curFailoverInd_].second));
    }
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





