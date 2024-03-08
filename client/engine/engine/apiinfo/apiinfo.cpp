#include "apiinfo.h"

#include <QSettings>

#include "utils/ws_assert.h"
#include "utils/logger.h"
#include "types/global_consts.h"

#ifdef Q_OS_WIN
#include "utils/wincryptutils.h"
#endif

namespace apiinfo {

ApiInfo::ApiInfo() : simpleCrypt_(SIMPLE_CRYPT_KEY)
{
}

api_responses::SessionStatus ApiInfo::getSessionStatus() const
{
    WS_ASSERT(isSessionStatusInit_);
    return sessionStatus_;
}

void ApiInfo::setSessionStatus(const api_responses::SessionStatus &value)
{
    isSessionStatusInit_ = true;
    sessionStatus_ = value;
    QSettings settings;
    settings.setValue("userId", sessionStatus_.getUserId());    // need for uninstaller program for open post uninstall webpage
}

void ApiInfo::setLocations(const QVector<api_responses::Location> &value)
{
    isLocationsInit_ = true;
    locations_ = value;
    mergeWindflixLocations();
}

QVector<api_responses::Location> ApiInfo::getLocations() const
{
    return locations_;
}

QStringList ApiInfo::getForceDisconnectNodes() const
{
    return forceDisconnectNodes_;
}

void ApiInfo::setForceDisconnectNodes(const QStringList &value)
{
    isForceDisconnectInit_ = true;
    forceDisconnectNodes_ = value;
}

void ApiInfo::setServerCredentials(const ServerCredentials &serverCredentials)
{
    serverCredentials_ = serverCredentials;
}

ServerCredentials ApiInfo::getServerCredentials() const
{
    return serverCredentials_;
}

void ApiInfo::setServerCredentialsOpenVpn(const QString &username, const QString &password)
{
    serverCredentials_.setForOpenVpn(username, password);
}

void ApiInfo::setServerCredentialsIkev2(const QString &username, const QString &password)
{
    serverCredentials_.setForIkev2(username, password);
}

bool ApiInfo::isServerCredentialsOpenVpnInit() const
{
    return serverCredentials_.isOpenVpnInitialized();
}

bool ApiInfo::isServerCredentialsIkev2Init() const
{
    return serverCredentials_.isIkev2Initialized();
}

QString ApiInfo::getOvpnConfig() const
{
    WS_ASSERT(isOvpnConfigInit_);
    return ovpnConfig_;
}

void ApiInfo::setOvpnConfig(const QString &value)
{
    isOvpnConfigInit_ = true;
    ovpnConfig_ = value;
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

api_responses::PortMap ApiInfo::getPortMap() const
{
    WS_ASSERT(isPortMapInit_);
    return portMap_;
}

void ApiInfo::setPortMap(const api_responses::PortMap &portMap)
{
    isPortMapInit_ = true;
    portMap_ = portMap;
    checkPortMapForUnavailableProtocolAndFix();
}

void ApiInfo::setStaticIps(const api_responses::StaticIps &value)
{
    isStaticIpsInit_ = true;
    staticIps_ = value;
}

api_responses::StaticIps ApiInfo::getStaticIps() const
{
    return staticIps_;
}

void ApiInfo::saveToSettings()
{
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

bool ApiInfo::isEverythingInit() const
{
    return isSessionStatusInit_ && isLocationsInit_ && isForceDisconnectInit_  &&
           isOvpnConfigInit_ && isPortMapInit_ && isStaticIpsInit_ && serverCredentials_.isInitialized();
}

bool ApiInfo::loadFromSettings()
{
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
            isSessionStatusInit_ = true;
            isLocationsInit_ = true;
            isForceDisconnectInit_ = true;
            isOvpnConfigInit_ = true;
            isPortMapInit_ = true;
            isStaticIpsInit_ = true;
            checkPortMapForUnavailableProtocolAndFix();
            return true;
        }
    }
    return false;
}

void ApiInfo::mergeWindflixLocations()
{
    // Build a new list of server locations to merge, removing them from the old list.
    // Currently we merge all WindFlix locations into the corresponding global locations.
    QVector<api_responses::Location> locationsToMerge;
    QMutableVectorIterator<api_responses::Location> it(locations_);
    while (it.hasNext()) {
        api_responses::Location &location = it.next();
        if (location.getName().startsWith("WINDFLIX")) {
            locationsToMerge.append(location);
            it.remove();
        }
    }
    if (!locationsToMerge.size())
        return;

    // Map city names to locations for faster lookups.
    QHash<QString, api_responses::Location *> location_hash;
    for (auto &location: locations_) {
        for (int i = 0; i < location.groupsCount(); ++i)
        {
            const api_responses::Group group = location.getGroup(i);
            location_hash.insert(location.getCountryCode() + group.getCity(), &location);
        }
    }

    // Merge the locations.
    QMutableVectorIterator<api_responses::Location> itm(locationsToMerge);
    while (itm.hasNext()) {
        api_responses::Location &location = itm.next();
        const auto country_code = location.getCountryCode();

        for (int i = 0; i < location.groupsCount(); ++i)
        {
            api_responses::Group group = location.getGroup(i);
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

void ApiInfo::checkPortMapForUnavailableProtocolAndFix()
{
    portMap_.removeUnsupportedProtocols(types::Protocol::supportedProtocols());
}

void ApiInfo::clearAutoLoginCredentials()
{
    QSettings settings;
    if (settings.contains("username")) {
        settings.remove("username");
    }
    if (settings.contains("password")) {
        settings.remove("password");
    }
}

static QString getAutoLoginCredential(const QString &key)
{
    QString credential;

#ifdef Q_OS_WIN
    try {
        QSettings settings;
        if (settings.contains(key)) {
            std::string encoded = settings.value(key).toString().toStdString();
            const auto decrypted = wsl::WinCryptUtils::decrypt(encoded, wsl::WinCryptUtils::EncodeHex);
            credential = QString::fromStdWString(decrypted);
        }
    }
    catch (std::system_error& ex) {
        qCDebug(LOG_BASIC) << "ApiInfo::getAutoLoginCredential() -" << ex.what() << ex.code().value();
    }
#endif

    return credential;
}

QString ApiInfo::autoLoginUsername()
{
    return getAutoLoginCredential("username");
}

QString ApiInfo::autoLoginPassword()
{
    return getAutoLoginCredential("password");
}

} //namespace apiinfo
