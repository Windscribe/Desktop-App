#pragma once

#include <QWidget>
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "engine/networkdetectionmanager/inetworkdetectionmanager.h"
#include "engine/networkaccessmanager/networkaccessmanager.h"
#include "engine/ping/pingmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

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
            qDebug() << "Connect state changed: " << state;
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
    void getCurrentNetworkInterface(types::NetworkInterface &networkInterface) override
    {
        networkInterface.networkOrSsid = networkName_;
    }

    void setNetworkName(const QString &name)
    {
        if (networkName_ != name) {
            networkName_ = name;
            qDebug() << "Network name changed: " << networkName_;
        }
    }

    bool isOnline() override { return bOnline_; }
    void setOnline( bool bOnline)
    {
        if (bOnline != bOnline_) {
            bOnline_ = bOnline;
            qDebug() << "isOnline changed: " << bOnline_;
            emit onlineStateChanged(bOnline_);
        }
    }

private:
    bool bOnline_ = true;
    QString networkName_;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:

private:
    Ui::Widget *ui;
    ConnectStateController_moc *connectStateController_;
    NetworkDetectionManager_moc *networkDetectionManager_;
    NetworkAccessManager *accessManager_;
    PingManager *pingManager_;
    PingMultipleHosts *pingHosts_;

    void updateIps();
};
