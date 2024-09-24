#include "customovpnauthcredentialsstorage.h"

#include <QIODevice>
#include <QSettings>

CustomOvpnAuthCredentialsStorage::CustomOvpnAuthCredentialsStorage()
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
    auto it = hash_.find(ovpnFileName);
    if (it != hash_.end()) {
        it->username = username;
        it->password = password;
    } else {
        Credentials c;
        c.username = username;
        c.password = password;
        hash_[ovpnFileName] = c;
    }
}

void CustomOvpnAuthCredentialsStorage::setPrivKeyPassword(const QString &ovpnFileName, const QString &privKeyPassword)
{
    QMutexLocker locker(&mutex_);
    auto it = hash_.find(ovpnFileName);
    if (it != hash_.end()) {
        it->privKeyPassword = privKeyPassword;
    } else {
        Credentials c;
        c.privKeyPassword = privKeyPassword;
        hash_[ovpnFileName] = c;
    }
}

void CustomOvpnAuthCredentialsStorage::removeCredentials(const QString &ovpnFileName)
{
    QMutexLocker locker(&mutex_);
    auto it = hash_.find(ovpnFileName);
    if (it != hash_.end()) {
        it->username.clear();
        it->password.clear();
    }
}

void CustomOvpnAuthCredentialsStorage::removePrivKeyPassword(const QString &ovpnFileName)
{
    QMutexLocker locker(&mutex_);
    auto it = hash_.find(ovpnFileName);
    if (it != hash_.end()) {
        it->privKeyPassword.clear();
    }
}

void CustomOvpnAuthCredentialsStorage::removeUnusedCredentials(const QStringList & /*existingOvpnFileNames*/)
{
    QMutexLocker locker(&mutex_);
}

CustomOvpnAuthCredentialsStorage::Credentials CustomOvpnAuthCredentialsStorage::getAuthCredentials(const QString &ovpnFileName)
{
    QMutexLocker locker(&mutex_);
    auto it = hash_.find(ovpnFileName);
    if (it != hash_.end()) {
        return *it;
    } else {
        return Credentials();
    }
}

void CustomOvpnAuthCredentialsStorage::clearCredentials()
{
    QMutexLocker locker(&mutex_);
    hash_.clear();
}

void CustomOvpnAuthCredentialsStorage::readFromSettings()
{
    QMutexLocker locker(&mutex_);
    QSettings settings;
    if (settings.contains("customOvpnAuths")) {
        QByteArray arr = settings.value("customOvpnAuths").toByteArray();
        QDataStream in(&arr,QIODevice::ReadOnly);
        in >> hash_;
    }
}

void CustomOvpnAuthCredentialsStorage::writeToSettings()
{
    QMutexLocker locker(&mutex_);
    QByteArray arr;
    QDataStream out(&arr,QIODevice::WriteOnly);
    out << hash_;

    QSettings settings;
    settings.setValue("customOvpnAuths", arr);
}

QDataStream& operator<<(QDataStream &out, const CustomOvpnAuthCredentialsStorage::Credentials &c)
{
    out << c.username;
    out << c.password;
    out << c.privKeyPassword;
    return out;
}

QDataStream& operator>>(QDataStream &in, CustomOvpnAuthCredentialsStorage::Credentials &c)
{
    in >> c.username;
    in >> c.password;
    if (!in.atEnd()) {
        in >> c.privKeyPassword;
    }
    return in;
}
