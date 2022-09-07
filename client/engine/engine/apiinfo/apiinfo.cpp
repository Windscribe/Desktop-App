#include "apiinfo.h"
#include <QThread>
#include <QSettings>
#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "types/global_consts.h"

namespace apiinfo {

ApiInfo::ApiInfo() : simpleCrypt_(SIMPLE_CRYPT_KEY), threadId_(QThread::currentThreadId())
{
}

types::SessionStatus ApiInfo::getSessionStatus() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return sessionStatus_;
}

void ApiInfo::setSessionStatus(const types::SessionStatus &value)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    sessionStatus_ = value;
    QSettings settings;
    settings.setValue("userId", sessionStatus_.getUserId());    // need for uninstaller program for open post uninstall webpage
}

void ApiInfo::setLocations(const QVector<apiinfo::Location> &value)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    locations_ = value;
    mergeWindflixLocations();
}

QVector<apiinfo::Location> ApiInfo::getLocations() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return locations_;
}

QStringList ApiInfo::getForceDisconnectNodes() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return forceDisconnectNodes_;
}

void ApiInfo::setForceDisconnectNodes(const QStringList &value)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    forceDisconnectNodes_ = value;
}

void ApiInfo::setServerCredentials(const ServerCredentials &serverCredentials)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    serverCredentials_ = serverCredentials;
}

ServerCredentials ApiInfo::getServerCredentials() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return serverCredentials_;
}

QString ApiInfo::getOvpnConfig() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return ovpnConfig_;
}

void ApiInfo::setOvpnConfig(const QString &value)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    ovpnConfig_ = value;
    ovpnConfigSetTimestamp_ = QDateTime::currentDateTimeUtc();
}

// return empty string if auth hash not exist in the settings
QString ApiInfo::getAuthHash()
{
    QString authHash;
    QSettings settings2;
    authHash = settings2.value("authHash", "").toString();
    if (authHash.isEmpty())
    {
        // try load from settings of the app ver1
        QSettings settings1("Windscribe", "Windscribe");
        authHash = settings1.value("authHash", "").toString();
    }
    return authHash;
}

void ApiInfo::setAuthHash(const QString &authHash)
{
    QSettings settings;
    settings.setValue("authHash", authHash);
}

types::PortMap ApiInfo::getPortMap() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return portMap_;
}

void ApiInfo::setPortMap(const types::PortMap &portMap)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    portMap_ = portMap;
}

void ApiInfo::setStaticIps(const StaticIps &value)
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    staticIps_ = value;
}

StaticIps ApiInfo::getStaticIps() const
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    return staticIps_;
}

void ApiInfo::saveToSettings()
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << sessionStatus_ << locations_ << serverCredentials_ << ovpnConfig_ << portMap_ << staticIps_;
    }
    QSettings settings;
    settings.setValue("apiInfo", simpleCrypt_.encryptToString(arr));
    if (!sessionStatus_.getRevisionHash().isEmpty())
    {
        settings.setValue("revisionHash", sessionStatus_.getRevisionHash());
    }
    else
    {
        settings.remove("revisionHash");
    }
}

void ApiInfo::removeFromSettings()
{
    {
        QSettings settings;
        settings.remove("apiInfo");
        settings.remove("authHash");
    }
    // remove from first version too
    {
        QSettings settings1("Windscribe", "Windscribe");
        settings1.remove("apiInfo");
        settings1.remove("authHash");
    }
}

bool ApiInfo::loadFromSettings()
{
    WS_ASSERT(threadId_ == QThread::currentThreadId());
    QSettings settings;
    QString s = settings.value("apiInfo", "").toString();
    if (!s.isEmpty())
    {
        QByteArray arr = simpleCrypt_.decryptToByteArray(s);
        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic != magic_)
        {
            return false;
        }
        ds >> version;
        if (version > versionForSerialization_)
        {
            return false;
        }
        ds >> sessionStatus_ >> locations_ >> serverCredentials_ >> ovpnConfig_ >> portMap_ >> staticIps_;
        if (ds.status() == QDataStream::Ok)
        {
            forceDisconnectNodes_.clear();
            sessionStatus_.setRevisionHash(settings.value("revisionHash", "").toString());
            return true;
        }
    }
    return false;
}

void ApiInfo::mergeWindflixLocations()
{
    // Build a new list of server locations to merge, removing them from the old list.
    // Currently we merge all WindFlix locations into the corresponding global locations.
    QVector<apiinfo::Location> locationsToMerge;
    QMutableVectorIterator<apiinfo::Location> it(locations_);
    while (it.hasNext()) {
        apiinfo::Location &location = it.next();
        if (location.getName().startsWith("WINDFLIX")) {
            locationsToMerge.append(location);
            it.remove();
        }
    }
    if (!locationsToMerge.size())
        return;

    // Map city names to locations for faster lookups.
    QHash<QString, apiinfo::Location *> location_hash;
    for (auto &location: locations_) {
        for (int i = 0; i < location.groupsCount(); ++i)
        {
            const apiinfo::Group group = location.getGroup(i);
            location_hash.insert(location.getCountryCode() + group.getCity(), &location);
        }
    }

    // Merge the locations.
    QMutableVectorIterator<apiinfo::Location> itm(locationsToMerge);
    while (itm.hasNext()) {
        apiinfo::Location &location = itm.next();
        const auto country_code = location.getCountryCode();

        for (int i = 0; i < location.groupsCount(); ++i)
        {
            apiinfo::Group group = location.getGroup(i);
            group.setOverrideDnsHostName(location.getDnsHostName());

            auto target = location_hash.find(country_code + group.getCity());
            WS_ASSERT(target != location_hash.end());
            if (target != location_hash.end())
            {
                target.value()->addGroup(group);
            }
        }
        itm.remove();
    }
}

bool ApiInfo::ovpnConfigRefetchRequired() const
{
    return ovpnConfigSetTimestamp_.isValid() && (ovpnConfigSetTimestamp_.secsTo(QDateTime::currentDateTimeUtc()) >= 60*60*24);
}

} //namespace apiinfo
