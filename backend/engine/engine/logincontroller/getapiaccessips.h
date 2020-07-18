#ifndef GETAPIACCESSIPS_H
#define GETAPIACCESSIPS_H

#include <QObject>
#include "Engine/ServerApi/serverapi.h"

// make ApiAccessIps  request to random hardcoded IPs and return list of hosts or fail result
class GetApiAccessIps : public QObject
{
    Q_OBJECT
public:
    explicit GetApiAccessIps(QObject *parent, ServerAPI *serverAPI);
    void get();

signals:
    void finished(SERVER_API_RET_CODE retCode, const QStringList &hosts);

private slots:
    void onAccessIpsAnswer(SERVER_API_RET_CODE retCode, const QStringList &hosts, uint userRole);

private:
    ServerAPI *serverAPI_;
    uint serverApiUserRole_;
    QStringList hardcodedIps_;

    void makeRequestToRandomIP();
};

#endif // GETAPIACCESSIPS_H
