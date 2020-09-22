#include "servercredentials.h"

#include <QDataStream>

namespace apiinfo {

ServerCredentials::ServerCredentials() :
    bInitialized_(false)
{
}

ServerCredentials::ServerCredentials(const QString &usernameOpenVpn, const QString &passwordOpenVpn, const QString &usernameIkev2, const QString &passwordIkev2):
    bInitialized_(true), usernameOpenVpn_(usernameOpenVpn), passwordOpenVpn_(passwordOpenVpn), usernameIkev2_(usernameIkev2), passwordIkev2_(passwordIkev2)
{
}

ServerCredentials::ServerCredentials(const ProtoApiInfo::ServerCredentials &serverCredentials)
{
    usernameOpenVpn_ = QString::fromStdString(serverCredentials.openvpn_username());
    passwordOpenVpn_ = QString::fromStdString(serverCredentials.openvpn_password());
    usernameIkev2_ = QString::fromStdString(serverCredentials.ikev2_username());
    passwordIkev2_ = QString::fromStdString(serverCredentials.ikev2_password());
    bInitialized_ = true;
}

ProtoApiInfo::ServerCredentials ServerCredentials::getProtoBuf() const
{
    Q_ASSERT(bInitialized_);
    ProtoApiInfo::ServerCredentials sc;
    sc.set_openvpn_username(usernameOpenVpn_.toStdString());
    sc.set_openvpn_password(passwordOpenVpn_.toStdString());
    sc.set_ikev2_username(usernameIkev2_.toStdString());
    sc.set_ikev2_password(passwordIkev2_.toStdString());
    return sc;
}

bool ServerCredentials::isInitialized() const
{
    return bInitialized_ && !usernameOpenVpn_.isEmpty() && !passwordOpenVpn_.isEmpty() && !usernameIkev2_.isEmpty() && !passwordIkev2_.isEmpty();
}

QString ServerCredentials::usernameForOpenVpn() const
{
    Q_ASSERT(bInitialized_);
    return usernameOpenVpn_;
}

QString ServerCredentials::passwordForOpenVpn() const
{
    Q_ASSERT(bInitialized_);
    return passwordOpenVpn_;
}

QString ServerCredentials::usernameForIkev2() const
{
    Q_ASSERT(bInitialized_);
    return usernameIkev2_;
}

QString ServerCredentials::passwordForIkev2() const
{
    Q_ASSERT(bInitialized_);
    return passwordIkev2_;
}

void ServerCredentials::writeToStream(QDataStream &stream)
{
    stream << usernameOpenVpn_;
    stream << passwordOpenVpn_;
    stream << usernameIkev2_;
    stream << passwordIkev2_;
}

void ServerCredentials::readFromStream(QDataStream &stream, int revision)
{
    stream >> usernameOpenVpn_;
    stream >> passwordOpenVpn_;
    if (revision >= 3)
    {
        stream >> usernameIkev2_;
        stream >> passwordIkev2_;
        bInitialized_ = true;
    }
    else
    {
        bInitialized_ = false;
    }
}

} //namespace apiinfo
