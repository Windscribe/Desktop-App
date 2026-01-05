#include "persistentstate.h"

#include <QDataStream>
#include <QIODevice>
#include <QSettings>

#include "utils/log/categories.h"
#include "utils/simplecrypt.h"
#include "types/global_consts.h"

void PersistentState::load()
{
    QSettings settings;
    bool bLoaded = false;

    if (settings.contains("guiPersistentState")) {
        SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
        QString str = settings.value("guiPersistentState", "").toString();
        QByteArray arr = simpleCrypt.decryptToByteArray(str);

        QDataStream ds(&arr, QIODevice::ReadOnly);

        quint32 magic, version;
        ds >> magic;
        if (magic == magic_)
        {
            ds >> version;
            if (version <= versionForSerialization_)
            {
                ds >> state_;
                if (ds.status() == QDataStream::Ok)
                {
                    bLoaded = true;
                }
            }
        }
    }
    if (!bLoaded)
    {
        qCDebug(LOG_BASIC) << "Could not load GUI persistent state -- resetting to defaults";
        state_ = types::GuiPersistentState(); // reset to defaults
    }
    // remove the legacy key of settings
    settings.remove("persistentGuiSettings");
}

void PersistentState::save()
{
    QByteArray arr;
    {
        QDataStream ds(&arr, QIODevice::WriteOnly);
        ds << magic_;
        ds << versionForSerialization_;
        ds << state_;
    }

    SimpleCrypt simpleCrypt(SIMPLE_CRYPT_KEY);
    QSettings settings;
    settings.setValue("guiPersistentState", simpleCrypt.encryptToString(arr));
    settings.sync();
}

void PersistentState::setFirewallState(bool bFirewallOn)
{
    state_.isFirewallOn = bFirewallOn;
    save();
}

bool PersistentState::isFirewallOn() const
{
    return state_.isFirewallOn;
}

void PersistentState::setLocationsViewportHeight(int height)
{
    state_.locationsViewportHeight = height;
    save();
}

int PersistentState::locationsViewportHeight() const
{
    return state_.locationsViewportHeight;
}

void PersistentState::setFirstLogin(bool bFirstRun)
{
    state_.isFirstLogin = bFirstRun;
    save();
}

bool PersistentState::isFirstLogin()
{
    return state_.isFirstLogin;
}

void PersistentState::setIgnoreCpuUsageWarnings(bool isIgnore)
{
    state_.isIgnoreCpuUsageWarnings = isIgnore;
    save();
}

bool PersistentState::isIgnoreCpuUsageWarnings()
{
    return state_.isIgnoreCpuUsageWarnings;
}

void PersistentState::setLastLocation(const LocationID &lid)
{
    state_.lastLocation = lid;
    save();
}

LocationID PersistentState::lastLocation() const
{
    return state_.lastLocation;
}

void PersistentState::setLastExternalIp(const QString &ip)
{
    state_.lastExternalIp = ip;
    save();
}

QString PersistentState::lastExternalIp() const
{
    return state_.lastExternalIp;
}

void PersistentState::setNetworkWhitelist(const QVector<types::NetworkInterface> &list)
{
    state_.networkWhiteList = list;
    save();
}

QVector<types::NetworkInterface> PersistentState::networkWhitelist() const
{
    return state_.networkWhiteList;
}

PersistentState::PersistentState()
{
    load();
}

bool PersistentState::haveAppGeometry() const
{
    return !state_.appGeometry.isEmpty();
}

void PersistentState::setAppGeometry(const QByteArray &geometry)
{
    state_.appGeometry = geometry;
    save();
}

const QByteArray& PersistentState::appGeometry() const
{
    return state_.appGeometry;
}

bool PersistentState::havePreferencesWindowHeight() const
{
    return state_.preferencesWindowHeight > 0;
}

void PersistentState::setPreferencesWindowHeight(int height)
{
    state_.preferencesWindowHeight = height;
    save();
}

int PersistentState::preferencesWindowHeight() const
{
    return state_.preferencesWindowHeight;
}

LOCATION_TAB PersistentState::lastLocationTab() const
{
    return state_.lastLocationTab;
}

void PersistentState::setLastLocationTab(LOCATION_TAB tab)
{
    state_.lastLocationTab = tab;
    save();
}

bool PersistentState::isIgnoreLocationServicesDisabled() const
{
    return state_.isIgnoreLocationServicesDisabled;
}

void PersistentState::setIgnoreLocationServicesDisabled(bool suppress)
{
    state_.isIgnoreLocationServicesDisabled = suppress;
    save();
}

bool PersistentState::isIgnoreNotificationDisabled() const
{
    return state_.isIgnoreNotificationDisabled;
}

void PersistentState::setIgnoreNotificationDisabled(bool suppress)
{
    state_.isIgnoreNotificationDisabled = suppress;
    save();
}

void PersistentState::fromIni(QSettings &settings)
{
    QVector<types::NetworkInterface> networks = state_.networkWhiteList;
    QVector<types::NetworkInterface> out;

    settings.beginGroup(kIniNetworksProp);
    for (auto network : networks) {
        settings.beginGroup(network.networkOrSsid);
        network.trustType = NETWORK_TRUST_TYPE_fromString(settings.value(kIniNetworkTrustTypeProp, NETWORK_TRUST_TYPE_toString(network.trustType)).toString());
        settings.endGroup();
        out << network;
    }
    settings.endGroup();

    state_.networkWhiteList = out;
    save();
}

void PersistentState::toIni(QSettings &settings)
{
    settings.beginGroup(kIniNetworksProp);
    for (auto network : state_.networkWhiteList) {
        settings.beginGroup(network.networkOrSsid);
        settings.setValue(kIniNetworkTrustTypeProp, NETWORK_TRUST_TYPE_toString(network.trustType));
        settings.endGroup();
    }
    settings.endGroup();
    save();
}

void PersistentState::fromJson(const QJsonObject &json)
{
    QVector<types::NetworkInterface> out;
    QMap<QString, NETWORK_TRUST_TYPE> trustTypes;

    if (json.contains(kJsonNetworksProp) && json[kJsonNetworksProp].isArray()) {
        const QJsonArray arr = json[kJsonNetworksProp].toArray();
        for (int i = 0; i < arr.size(); i++) {
            out << types::NetworkInterface(arr[i].toObject());
        }
    }

    state_.networkWhiteList = out;
    save();
}

QJsonObject PersistentState::toJson()
{
    QJsonObject json;
    QJsonArray arr;

    for (auto network : state_.networkWhiteList) {
        arr.append(network.toJson(false));
    }

    json[kJsonNetworksProp] = arr;
    return json;
}
