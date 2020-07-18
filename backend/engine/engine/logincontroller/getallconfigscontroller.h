#ifndef GETALLCONFIGSCONTROLLER_H
#define GETALLCONFIGSCONTROLLER_H

#include "engine/serverapi/serverapi.h"

// helper class for LoginController
class GetAllConfigsController : public QObject
{
    Q_OBJECT
public:
    GetAllConfigsController(QObject *parent);

    void putServerLocationsAnswer(SERVER_API_RET_CODE retCode, QVector<QSharedPointer<ServerLocation> > serverLocations, const QStringList &forceDisconnectNodes);
    void putServerCredentialsOpenVpnAnswer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword);
    void putServerCredentialsIkev2Answer(SERVER_API_RET_CODE retCode, const QString &radiusUsername, const QString &radiusPassword);
    void putServerConfigsAnswer(SERVER_API_RET_CODE retCode, QByteArray config);
    void putPortMapAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<PortMap> portMap);
    void putStaticIpsAnswer(SERVER_API_RET_CODE retCode, QSharedPointer<StaticIpsLocation> staticIpsLocation);

    // public variables for access from LoginController class
    QVector<QSharedPointer<ServerLocation> > serverLocations_;
    QStringList forceDisconnectNodes_;

    QByteArray ovpnConfig_;

    QSharedPointer<PortMap> portMap_;
    QSharedPointer<StaticIpsLocation> staticIpsLocation_;

    ServerCredentials getServerCredentials();

signals:
    void allConfigsReceived(SERVER_API_RET_CODE retCode);

private:
    bool bServerLocationsAnswerReceived_;
    bool bServerCredentialsAnswerOpenVpnReceived_;
    bool bServerCredentialsAnswerIkev2Received_;
    bool bServerConfigsAnswerReceived_;
    bool bPortMapAnswerReceived_;
    bool bStaticIpsAnswerReceived_;

    QString radiusUsernameOpenVpn_;
    QString radiusPasswordOpenVpn_;
    QString radiusUsernameIkev2_;
    QString radiusPasswordIkev2_;

    SERVER_API_RET_CODE serverLocationsRetCode_;
    SERVER_API_RET_CODE serverCredentialsOpenVpnRetCode_;
    SERVER_API_RET_CODE serverCredentialsIkev2RetCode_;
    SERVER_API_RET_CODE serverConfigsRetCode_;
    SERVER_API_RET_CODE portMapRetCode_;
    SERVER_API_RET_CODE staticIpsRetCode_;

    void handleComplete();
};

#endif // GETALLCONFIGSCONTROLLER_H
