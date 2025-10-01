#pragma once

#include <QString>
#include <QHash>
#include <QDataStream>
#include <QMutex>


class CustomOvpnAuthCredentialsStorage
{
public:
    CustomOvpnAuthCredentialsStorage();
    ~CustomOvpnAuthCredentialsStorage();

    void setAuthCredentials(const QString &ovpnFileName, const QString &username, const QString &password);
    void setPrivKeyPassword(const QString &ovpnFileName, const QString &password);
    void removeCredentials(const QString &ovpnFileName);
    void removePrivKeyPassword(const QString &ovpnFileName);
    void removeUnusedCredentials(const QStringList &existingOvpnFileNames);

    void clearCredentials();

    struct Credentials
    {
        QString username;
        QString password;
        QString privKeyPassword;
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
