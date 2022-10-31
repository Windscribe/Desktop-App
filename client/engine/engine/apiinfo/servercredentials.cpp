#include "servercredentials.h"

#include <QDataStream>
#include "utils/ws_assert.h"

namespace apiinfo {

ServerCredentials::ServerCredentials() :
    bIkev2Initialized_(true),
    bOpenVpnInitialized_(true)
{
}

ServerCredentials::ServerCredentials(const QString &usernameOpenVpn, const QString &passwordOpenVpn,
                                     const QString &usernameIkev2, const QString &passwordIkev2) :
    bIkev2Initialized_(true),
    bOpenVpnInitialized_(true),
    usernameOpenVpn_(usernameOpenVpn),
    passwordOpenVpn_(passwordOpenVpn),
    usernameIkev2_(usernameIkev2),
    passwordIkev2_(passwordIkev2)
{
}

bool ServerCredentials::isInitialized() const
{
    return bIkev2Initialized_ && bOpenVpnInitialized_;
}

QString ServerCredentials::usernameForOpenVpn() const
{
    WS_ASSERT(bOpenVpnInitialized_);
    return usernameOpenVpn_;
}

QString ServerCredentials::passwordForOpenVpn() const
{
    WS_ASSERT(bOpenVpnInitialized_);
    return passwordOpenVpn_;
}

QString ServerCredentials::usernameForIkev2() const
{
    WS_ASSERT(bIkev2Initialized_);
    return usernameIkev2_;
}

QString ServerCredentials::passwordForIkev2() const
{
    WS_ASSERT(bIkev2Initialized_);
    return passwordIkev2_;
}

void ServerCredentials::setForOpenVpn(const QString &username, const QString &password)
{
    bOpenVpnInitialized_ = true;
    usernameOpenVpn_ = username;
    passwordOpenVpn_ = password;
}

void ServerCredentials::setForIkev2(const QString &username, const QString &password)
{
    bIkev2Initialized_ = true;
    usernameIkev2_ = username;
    passwordIkev2_ = password;
}

bool ServerCredentials::isIkev2Initialized() const
{
    return bIkev2Initialized_;
}

bool ServerCredentials::isOpenVpnInitialized() const
{
    return bOpenVpnInitialized_;
}

QDataStream& operator <<(QDataStream &stream, const ServerCredentials &s)
{
    WS_ASSERT(s.isInitialized());
    stream << s.versionForSerialization_;
    stream << s.usernameOpenVpn_ << s.passwordOpenVpn_ << s.usernameIkev2_ << s.passwordIkev2_;
    return stream;
}

QDataStream& operator >>(QDataStream &stream, ServerCredentials &s)
{
    quint32 version;
    stream >> version;
    if (version > s.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        s.bIkev2Initialized_ = false;
        s.bOpenVpnInitialized_ = false;
        return stream;
    }

    stream >> s.usernameOpenVpn_ >> s.passwordOpenVpn_ >> s.usernameIkev2_ >> s.passwordIkev2_;
    s.bIkev2Initialized_ = true;
    s.bOpenVpnInitialized_ = true;

    return stream;
}

} //namespace apiinfo
