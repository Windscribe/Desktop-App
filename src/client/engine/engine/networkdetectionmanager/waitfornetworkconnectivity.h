#pragma once

#include <QObject>
#include <QTimer>
#include "inetworkdetectionmanager.h"

// util class allowing to wait until the network connectivity
class WaitForNetworkConnectivity : public QObject
{
    Q_OBJECT
public:
    explicit WaitForNetworkConnectivity(QObject *parent, INetworkDetectionManager *networkDetectionManager);

    void wait(int maxTime);

signals:
    void timeoutExpired();
    void connectivityOnline();

private slots:
    void onOnlineStateChanged(bool isOnline);
    void onTimer();

private:
    INetworkDetectionManager *networkDetectionManager_;
    QTimer *timer_;

};

