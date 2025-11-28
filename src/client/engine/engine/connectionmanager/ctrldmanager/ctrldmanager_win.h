#pragma once

#include <QProcess>
#include "ictrldmanager.h"

class CtrldManager_win : public ICtrldManager
{
    Q_OBJECT
public:
    explicit CtrldManager_win(QObject *parent, bool isCreateLog);
    virtual ~CtrldManager_win();

    bool runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains);
    void killProcess();
    QString listenIp() const;

    static void terminateWindscribeCtrldProcesses();

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onReadyReadStandardOutput();
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    QProcess *process_;
    QString ctrldExePath_;
    QString logPath_;
    bool bProcessStarted_;
    QString listenIp_;
    QByteArray inputArr_;

    QString getNextStringFromInputBuffer(bool &bSuccess, int &outSize);
    QString getAvailableIp();
};


