#pragma once

#include <QObject>
#include <QProcess>
#include "engine/helper/ihelper.h"

// Managing the launch of the external ctrld utility
class CtrldManager : public QObject
{
    Q_OBJECT
public:
    explicit CtrldManager(QObject *parent, IHelper *helper);
    virtual ~CtrldManager();

    bool runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains);
    void killProcess();
    QString listenIp() const;

signals:
    void ctrldFinished();
    void ctrldStarted();


private:
    IHelper *helper_;
    QString     ctrldExePath_;
    QString     logPath_;
    bool bProcessStarted_;
    QString listenIp_;

    QString getAvailableIp();
};


