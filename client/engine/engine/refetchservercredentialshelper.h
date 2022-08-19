#ifndef REFETCHSERVERCREDENTIALSHELPER_H
#define REFETCHSERVERCREDENTIALSHELPER_H

#include <QObject>
#include <QTimer>
#include "engine/serverapi/serverapi.h"
#include "engine/apiinfo/servercredentials.h"

class RefetchServerCredentialsHelper : public QObject
{
    Q_OBJECT

public:
    explicit RefetchServerCredentialsHelper(QObject *parent, const QString &authHash, ServerAPI *serverAPI);
    virtual ~RefetchServerCredentialsHelper();
    void startRefetch();

signals:
    void finished(bool success, const apiinfo::ServerCredentials &serverCredentials, const QString &serverConfig);

private slots:
    void onTimerWaitServerAPIReady();
    void onServerCredentialsAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword, PROTOCOL protocol, uint userRole);
    void onServerConfigsAnswer(SERVER_API_RET_CODE retCode, const QString &config, uint userRole);

private:
    QString authHash_;
    ServerAPI *serverAPI_;
    uint serverApiUserRole_;
    QTimer timerWaitServerAPIReady_;
    int refetchServerCredentialsState_;
    bool isOpenVpnProtocolReceived_;
    bool isIkev2ProtocolReceived_;
    bool isServerConfigsAnswerReceived_;
    SERVER_API_RET_CODE retCodeOpenVpn_;
    SERVER_API_RET_CODE retCodeIkev2_;
    SERVER_API_RET_CODE retCodeServerConfigs_;
    QString radiusUsernameOpenVpn_;
    QString radiusPasswordOpenVpn_;
    QString radiusUsernameIkev2_;
    QString radiusPasswordIkev2_;
    QString serverConfig_;

    void checkFinished();
    void fetchServerCredentials();
    void putFail();
};

#endif // REFETCHSERVERCREDENTIALSHELPER_H
