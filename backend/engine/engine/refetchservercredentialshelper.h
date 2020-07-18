#ifndef REFETCHSERVERCREDENTIALSHELPER_H
#define REFETCHSERVERCREDENTIALSHELPER_H

#include <QObject>
#include <QTimer>
#include "engine/serverapi/serverapi.h"

class RefetchServerCredentialsHelper : public QObject
{
    Q_OBJECT

public:
    explicit RefetchServerCredentialsHelper(QObject *parent, const QString &authHash, ServerAPI *serverAPI);
    virtual ~RefetchServerCredentialsHelper();
    void startRefetch();

signals:
    void finished(bool success, const ServerCredentials &serverCredentials);

private slots:
    void onTimerWaitServerAPIReady();
    void onServerCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword, ProtocolType protocol, uint userRole);


private:
    QString authHash_;
    ServerAPI *serverAPI_;
    uint serverApiUserRole_;
    QTimer timerWaitServerAPIReady_;
    int refetchServerCredentialsState_;
    bool isOpenVpnProtocolReceived_;
    bool isIkev2ProtocolReceived_;
    SERVER_API_RET_CODE retCodeOpenVpn_;
    SERVER_API_RET_CODE retCodeIkev2_;
    QString radiusUsernameOpenVpn_;
    QString radiusPasswordOpenVpn_;
    QString radiusUsernameIkev2_;
    QString radiusPasswordIkev2_;

    void putFail();
};

#endif // REFETCHSERVERCREDENTIALSHELPER_H
