#include "servercredentials.h"

#include <QDataStream>

namespace types {

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

} //namespace types
