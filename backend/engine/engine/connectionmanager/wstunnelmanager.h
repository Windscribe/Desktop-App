#ifndef WSTUNNELMANAGER_H
#define WSTUNNELMANAGER_H

#include <QObject>
#include <QProcess>

class WstunnelManager : public QObject
{
    Q_OBJECT
public:
    explicit WstunnelManager(QObject *parent = 0);
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
    QProcess    *process_;
    QString     wstunelExePath_;
    bool bProcessStarted_;
    QByteArray inputArr_;
    bool bFirstMarketLineAfterStart_;

    static constexpr unsigned int DEFAULT_PORT = 1194;
    unsigned int port_;

    QString getNextStringFromInputBuffer(bool &bSuccess, int &outSize);

};

#endif // WSTUNNELMANAGER_H
