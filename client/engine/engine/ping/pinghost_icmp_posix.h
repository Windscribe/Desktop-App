#pragma once

#include "ipinghost.h"
#include <QProcess>

// todo proxy support for icmp ping
class PingHost_ICMP_posix : public IPingHost
{
    Q_OBJECT
public:
    explicit PingHost_ICMP_posix(QObject *parent, const QString &ip);

    void ping() override;

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString ip_;
    QProcess *process_ = nullptr;

    int extractTimeMs(const QString &str);
};
