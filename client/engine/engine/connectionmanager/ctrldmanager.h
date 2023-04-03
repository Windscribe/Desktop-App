#pragma once

#include <QObject>
#include <QProcess>

// Managing the launch of the external ctrld utility
class CtrldManager : public QObject
{
    Q_OBJECT
public:
    explicit CtrldManager(QObject *parent = 0);
    virtual ~CtrldManager();

    bool runProcess(const QString &upstream1, const QString &upstream2, const QStringList &domains);
    void killProcess();
    QString listenIp() const;

signals:
    void ctrldFinished();
    void ctrldStarted();

private slots:
    void onProcessStarted();
    void onProcessFinished();
    void onReadyReadStandardOutput();
    void onProcessErrorOccurred(QProcess::ProcessError error);

private:
    QProcess    *process_;
    QString     ctrldExePath_;
    QString     logPath_;
    bool bProcessStarted_;
    QByteArray inputArr_;
    QString listenIp_;

    QString getNextStringFromInputBuffer(bool &bSuccess, int &outSize);
    QString getAvailableIp();
};


