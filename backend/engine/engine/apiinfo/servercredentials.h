#ifndef APIINFO_SERVERCREDENTIALS_H
#define APIINFO_SERVERCREDENTIALS_H

#include <QString>
#include "ipc/generated_proto/apiinfo.pb.h"

namespace ApiInfo {

class ServerCredentials
{
public:
    ServerCredentials();
    ServerCredentials(const QString &usernameOpenVpn, const QString &passwordOpenVpn, const QString &usernameIkev2, const QString &passwordIkev2);
    ServerCredentials(const ProtoApiInfo::ServerCredentials &serverCredentials);

    ProtoApiInfo::ServerCredentials getProtoBuf() const;

    bool isInitialized() const;
    QString usernameForOpenVpn() const;
    QString passwordForOpenVpn() const;
    QString usernameForIkev2() const;
    QString passwordForIkev2() const;

    void writeToStream(QDataStream &stream);
    void readFromStream(QDataStream &stream, int revision);

private:
    bool bInitialized_;
    QString usernameOpenVpn_;
    QString passwordOpenVpn_;
    QString usernameIkev2_;
    QString passwordIkev2_;
};

} //namespace ApiInfo

#endif // APIINFO_SERVERCREDENTIALS_H
