#ifndef TYPES_SERVERCREDENTIALS_H
#define TYPES_SERVERCREDENTIALS_H

#include <QString>
#include <QDataStream>

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

    friend QDataStream& operator <<(QDataStream &stream, const ServerCredentials &s)
    {
        Q_ASSERT(s.bInitialized_);
        stream << versionForSerialization_;
        stream << s.usernameOpenVpn_ << s.passwordOpenVpn_ << s.usernameIkev2_ << s.passwordIkev2_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, ServerCredentials &s)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version != versionForSerialization_)
        {
            s.bInitialized_ = false;
            return stream;
        }

        stream >> s.usernameOpenVpn_ >> s.passwordOpenVpn_ >> s.usernameIkev2_ >> s.passwordIkev2_;
        s.bInitialized_ = true;

        return stream;
    }

private:
    bool bInitialized_;
    QString usernameOpenVpn_;
    QString passwordOpenVpn_;
    QString usernameIkev2_;
    QString passwordIkev2_;
    static constexpr quint32 versionForSerialization_ = 1;

};

} //namespace types

#endif // TYPES_SERVERCREDENTIALS_H
