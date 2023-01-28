#ifndef CUSTOMOVPNAUTHCREDENTIALSSTORAGE_H
#define CUSTOMOVPNAUTHCREDENTIALSSTORAGE_H

#include <QString>
#include <QHash>
#include <QDataStream>
#include <QRecursiveMutex>


class CustomOvpnAuthCredentialsStorage
{
public:
    CustomOvpnAuthCredentialsStorage();
    ~CustomOvpnAuthCredentialsStorage();

    void setAuthCredentials(const QString &ovpnFileName, const QString &username, const QString &password);
    void removeCredentials(const QString &ovpnFileName);
    void removeUnusedCredentials(const QStringList &existingOvpnFileNames);

    struct Credentials
    {
        QString username;
        QString password;
    };

    Credentials getAuthCredentials(const QString &ovpnFileName);

private:
    void readFromSettings();
    void writeToSettings();

    QHash<QString, Credentials> hash_;
    QRecursiveMutex mutex_;

};

QDataStream& operator<<(QDataStream &out, const CustomOvpnAuthCredentialsStorage::Credentials &c);
QDataStream& operator>>(QDataStream &in, CustomOvpnAuthCredentialsStorage::Credentials &c);

#endif // CUSTOMOVPNAUTHCREDENTIALSSTORAGE_H
