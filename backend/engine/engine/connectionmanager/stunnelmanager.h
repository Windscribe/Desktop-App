#ifndef STUNNELMANAGER_H
#define STUNNELMANAGER_H

#include <QObject>
#include <QProcess>

class StunnelManager : public QObject
{
    Q_OBJECT
public:
    explicit StunnelManager(QObject *parent = 0);
    virtual ~StunnelManager();

    bool setConfig(const QString &hostname, uint port);
    void runProcess();
    void killProcess();

    unsigned int getStunnelPort();

signals:
    void stunnelFinished();

private slots:
    void onStunnelProcessFinished();

private:
    QString path_;
    QProcess    *process_;
    QString     stunelExePath_;
    bool bProcessStarted_;

    const unsigned int DEFAULT_PORT = 1194;
    unsigned int portForStunnel_;

    bool makeConfigFile(const QString &hostname, uint port);
};

#endif // STUNNELMANAGER_H
