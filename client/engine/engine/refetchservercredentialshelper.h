#pragma once

#include <QObject>
#include <QTimer>
#include "engine/serverapi/serverapi.h"
#include "engine/apiinfo/servercredentials.h"

class RefetchServerCredentialsHelper : public QObject
{
    Q_OBJECT

public:
    explicit RefetchServerCredentialsHelper(QObject *parent, const QString &authHash, server_api::ServerAPI *serverAPI);
    virtual ~RefetchServerCredentialsHelper();
    void startRefetch();

signals:
    void finished(bool success, const apiinfo::ServerCredentials &serverCredentials, const QString &serverConfig);

private slots:
   void onServerCredentialsAnswer();
    void onServerConfigsAnswer();

private:
    QString authHash_;
    server_api::ServerAPI *serverAPI_;
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

