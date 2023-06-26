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

    bool setConfig(const QString &hostname, uint port);
    bool runProcess();
    void killProcess();

    unsigned int getStunnelPort();

signals:
    void stunnelFinished();

private slots:
    void onStunnelProcessFinished();

private:
    static constexpr unsigned int DEFAULT_PORT = 1194;

    IHelper *helper_;
    unsigned int portForStunnel_;
    bool bProcessStarted_;

#ifdef Q_OS_WIN
    QString path_;
    QProcess    *process_;
    QString     stunnelExePath_;

    bool makeConfigFile(const QString &hostname, uint port);
#endif
};
