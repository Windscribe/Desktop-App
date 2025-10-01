#pragma once

#include <QObject>
#include <QProcess>
#include "engine/helper/helper.h"

class StunnelManager : public QObject
{
    Q_OBJECT
public:
    explicit StunnelManager(QObject *parent, Helper *helper);
    virtual ~StunnelManager();

    bool runProcess(const QString &hostname, unsigned int port, bool isExtraPadding);
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

    Helper *helper_;
    unsigned int port_;
    bool bProcessStarted_;
    QString path_;
    QProcess *process_;
    QString stunnelExePath_;
};
