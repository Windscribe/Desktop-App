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

    bool runProcess(const QString &hostname, unsigned int port, bool isUdp);
    void killProcess();

    unsigned int getPort();

signals:
    void wstunnelFinished();
    void wstunnelStarted();

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onReadyReadStandardOutput();
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    IHelper *helper_;
    QProcess    *process_;
    QString     wstunnelExePath_;
    bool bProcessStarted_;
    QByteArray inputArr_;
    bool bFirstMarketLineAfterStart_;

    static constexpr unsigned int DEFAULT_PORT = 1194;
    unsigned int port_;

    QString getNextStringFromInputBuffer(bool &bSuccess, int &outSize);

};
