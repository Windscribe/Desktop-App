#pragma once

#include <QList>
#include <QObject>
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"


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


class RequestExecuterViaFailover_test : public QObject
{
    Q_OBJECT

public:
    RequestExecuterViaFailover_test();

private slots:
    void init();
    void cleanup();

    void testEchFailover();

private:
    void testDynamicFailover();
    void testFailoverFailed();
    void testRequestDeleted();
    void testConnectStateChanged();
    void testMultiIpFailover();


    NetworkAccessManager *networkAccessManager_;
    ConnectStateController_moc *connectStateController_;

};





