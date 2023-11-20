#pragma once

#include <QObject>
#include <QProcess>
#include "engine/helper/ihelper.h"

class WstunnelManager : public QObject
{
    Q_OBJECT
public:
    explicit WstunnelManager(QObject *parent, IHelper *helper);
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

    IHelper *helper_;
    QProcess *process_;
    QString wstunnelExePath_;
    bool bProcessStarted_;
    unsigned int port_;
};
