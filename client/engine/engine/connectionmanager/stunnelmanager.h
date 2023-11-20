#pragma once

#include <QObject>
#include <QProcess>
#include "engine/helper/ihelper.h"

class StunnelManager : public QObject
{
    Q_OBJECT
public:
    explicit StunnelManager(QObject *parent, IHelper *helper);
    virtual ~StunnelManager();

    bool runProcess(const QString &hostname, unsigned int port);
    void killProcess();

    unsigned int getPort();

signals:
    void stunnelStarted();
    void stunnelFinished();

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onProcessReadyRead();
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    static constexpr unsigned int kDefaultPort = 1194;

    IHelper *helper_;
    unsigned int port_;
    bool bProcessStarted_;
    QString path_;
    QProcess *process_;
    QString stunnelExePath_;
};
