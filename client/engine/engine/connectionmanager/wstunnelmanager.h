#pragma once

#include <QObject>
#include <QProcess>
#include "engine/helper/helper.h"

class WstunnelManager : public QObject
{
    Q_OBJECT
public:
    explicit WstunnelManager(QObject *parent, Helper *helper);
    virtual ~WstunnelManager();

    bool runProcess(const QString &hostname, unsigned int port);
    void killProcess();

    unsigned int getPort();

signals:
    void wstunnelFinished();
    void wstunnelStarted();

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onProcessReadyRead();
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    static constexpr unsigned int kDefaultPort = 1194;

    Helper *helper_;
    QProcess *process_;
    QString wstunnelExePath_;
    bool bProcessStarted_;
    unsigned int port_;
};
