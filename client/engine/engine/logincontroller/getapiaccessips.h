#ifndef GETAPIACCESSIPS_H
#define GETAPIACCESSIPS_H

#include <QObject>
#include "engine/serverapi/serverapi.h"

// make ApiAccessIps  request to random hardcoded IPs and return list of hosts or fail result
class GetApiAccessIps : public QObject
{
    Q_OBJECT
public:
    explicit GetApiAccessIps(QObject *parent, server_api::ServerAPI *serverAPI);
    void get();

signals:
    void finished(SERVER_API_RET_CODE retCode, const QStringList &hosts);

private slots:
    void onAccessIpsAnswer();

private:
    server_api::ServerAPI *serverAPI_;
    QStringList hardcodedIps_;

    void makeRequestToRandomIP();
};

#endif // GETAPIACCESSIPS_H
