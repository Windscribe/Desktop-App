#ifndef HOSTNAMESRESOLVER_H
#define HOSTNAMESRESOLVER_H

#include <QHostInfo>
#include <QObject>
#include "pingtime.h"

namespace locationsmodel {

// Helper class for LocationsModel. Makes a DNS resolution for hostnames from custom configs.
// It also stores and processes information about pings for hostnames.
class HostnamesResolver : public QObject
{
    Q_OBJECT
public:
    explicit HostnamesResolver(QObject *parent = nullptr);

    void clear();
    void setHostnames(const QStringList &hostnames);
    QStringList allIps() const;
    QString hostnameForIp(const QString &ip) const;

    // ping info functions
    void setLatencyForIp(const QString &ip, PingTime pingTime);
    void clearLatencyInfo();
    PingTime getLatencyForHostname(const QString &hostname) const;

signals:
    void allResolved();

private slots:
    void onResolved(const QString &hostname, const QHostInfo &hostInfo, void *userPointer);

private:
    struct HostnameInfo
    {
        bool isResolved;
        QStringList ips;

        HostnameInfo() : isResolved(false) {}
    };

    QHash<QString, HostnameInfo> hash_;

    QHash<QString, int> hashLatencies_;

};

} //namespace locationsmodel

#endif // HOSTNAMESRESOLVER_H
