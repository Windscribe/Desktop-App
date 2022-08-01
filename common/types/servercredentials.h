#ifndef TYPES_SERVERCREDENTIALS_H
#define TYPES_SERVERCREDENTIALS_H

#include <QString>

namespace types {

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

private:
    bool bInitialized_;
    QString usernameOpenVpn_;
    QString passwordOpenVpn_;
    QString usernameIkev2_;
    QString passwordIkev2_;
};

} //namespace types

#endif // TYPES_SERVERCREDENTIALS_H
