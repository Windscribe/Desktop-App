#include "servercredentials.h"

#include <QDataStream>
#include "utils/ws_assert.h"

namespace apiinfo {

ServerCredentials::ServerCredentials() :
    bInitialized_(false)
{
}

ServerCredentials::ServerCredentials(const QString &usernameOpenVpn, const QString &passwordOpenVpn,
                                     const QString &usernameIkev2, const QString &passwordIkev2) :
    bInitialized_(true), usernameOpenVpn_(usernameOpenVpn), passwordOpenVpn_(passwordOpenVpn),
    usernameIkev2_(usernameIkev2), passwordIkev2_(passwordIkev2)
{
}

bool ServerCredentials::isInitialized() const
{
    return bInitialized_ && !usernameOpenVpn_.isEmpty() && !passwordOpenVpn_.isEmpty() && !usernameIkev2_.isEmpty() && !passwordIkev2_.isEmpty();
}

QString ServerCredentials::usernameForOpenVpn() const
{
    WS_ASSERT(bInitialized_);
    return usernameOpenVpn_;
}

QString ServerCredentials::passwordForOpenVpn() const
{
    WS_ASSERT(bInitialized_);
    return passwordOpenVpn_;
}

QString ServerCredentials::usernameForIkev2() const
{
    WS_ASSERT(bInitialized_);
    return usernameIkev2_;
}

QString ServerCredentials::passwordForIkev2() const
{
    WS_ASSERT(bInitialized_);
    return passwordIkev2_;
}

QDataStream& operator <<(QDataStream &stream, const ServerCredentials &s)
{
    WS_ASSERT(s.bInitialized_);
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
        s.bInitialized_ = false;
        return stream;
    }

    stream >> s.usernameOpenVpn_ >> s.passwordOpenVpn_ >> s.usernameIkev2_ >> s.passwordIkev2_;
    s.bInitialized_ = true;

    return stream;
}

} //namespace apiinfo
