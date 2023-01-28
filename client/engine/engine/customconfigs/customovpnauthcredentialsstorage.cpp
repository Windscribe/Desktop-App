#include "customovpnauthcredentialsstorage.h"

#include <QSettings>

CustomOvpnAuthCredentialsStorage::CustomOvpnAuthCredentialsStorage() : mutex_(QRecursiveMutex())
{
    readFromSettings();
}

CustomOvpnAuthCredentialsStorage::~CustomOvpnAuthCredentialsStorage()
{
    writeToSettings();
}

void CustomOvpnAuthCredentialsStorage::setAuthCredentials(const QString &ovpnFileName, const QString &username, const QString &password)
{
    QMutexLocker locker(&mutex_);
    Credentials c;
    c.username = username;
    c.password = password;
    hash_[ovpnFileName] = c;
}

void CustomOvpnAuthCredentialsStorage::removeCredentials(const QString &ovpnFileName)
{
    QMutexLocker locker(&mutex_);
    hash_.remove(ovpnFileName);
}

void CustomOvpnAuthCredentialsStorage::removeUnusedCredentials(const QStringList & /*existingOvpnFileNames*/)
{
    QMutexLocker locker(&mutex_);
}

CustomOvpnAuthCredentialsStorage::Credentials CustomOvpnAuthCredentialsStorage::getAuthCredentials(const QString &ovpnFileName)
{
    QMutexLocker locker(&mutex_);
    auto it = hash_.find(ovpnFileName);
    if (it != hash_.end())
    {
        return *it;
    }
    else
    {
        return Credentials();
    }
}

void CustomOvpnAuthCredentialsStorage::readFromSettings()
{
    QMutexLocker locker(&mutex_);
    QSettings settings;
    if (settings.contains("customOvpnAuths"))
    {
        QByteArray arr = settings.value("customOvpnAuths").toByteArray();
        QDataStream in(&arr,QIODeviceBase::ReadOnly);
        in >> hash_;
    }
}

void CustomOvpnAuthCredentialsStorage::writeToSettings()
{
    QMutexLocker locker(&mutex_);
    QByteArray arr;
    QDataStream out(&arr,QIODeviceBase::WriteOnly);
    out << hash_;

    QSettings settings;
    settings.setValue("customOvpnAuths", arr);
}

QDataStream& operator<<(QDataStream &out, const CustomOvpnAuthCredentialsStorage::Credentials &c)
{
    out << c.username;
    out << c.password;
    return out;
}

QDataStream& operator>>(QDataStream &in, CustomOvpnAuthCredentialsStorage::Credentials &c)
{
    in >> c.username;
    in >> c.password;
    return in;
}
