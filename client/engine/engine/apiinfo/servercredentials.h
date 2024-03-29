#pragma once

#include <QString>
#include <QDataStream>

namespace apiinfo {

class ServerCredentials
{
public:
    ServerCredentials();
    ServerCredentials(const QString &usernameOpenVpn, const QString &passwordOpenVpn, const QString &usernameIkev2, const QString &passwordIkev2);

    bool isInitialized() const;
    QString usernameForOpenVpn() const;
    QString passwordForOpenVpn() const;
    QString usernameForIkev2() const;
    QString passwordForIkev2() const;

    void setForOpenVpn(const QString &username, const QString &password);
    void setForIkev2(const QString &username, const QString &password);

    bool isIkev2Initialized() const;
    bool isOpenVpnInitialized() const;

    friend QDataStream& operator <<(QDataStream &stream, const ServerCredentials &s);
    friend QDataStream& operator >>(QDataStream &stream, ServerCredentials &s);

private:
    bool bIkev2Initialized_;
    bool bOpenVpnInitialized_;


    QString usernameOpenVpn_;
    QString passwordOpenVpn_;
    QString usernameIkev2_;
    QString passwordIkev2_;

    static constexpr quint32 versionForSerialization_ = 1;

};

} //namespace apiinfo

