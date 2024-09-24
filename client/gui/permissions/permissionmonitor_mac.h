#pragma once

#include <QObject>

class PermissionMonitor_mac : public QObject
{
    Q_OBJECT
public:
    explicit PermissionMonitor_mac(QObject *parent = nullptr);
    ~PermissionMonitor_mac();

    void onLocationPermissionUpdated();

signals:
    void locationPermissionUpdated();
};
