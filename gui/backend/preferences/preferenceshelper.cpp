#include "preferenceshelper.h"
#include "utils/languagesutil.h"
#include "version/appversion.h"

#if defined(Q_OS_WINDOWS)
#include "utils/winutils.h"
#endif

PreferencesHelper::PreferencesHelper(QObject *parent) : QObject(parent),
    isWifiSharingSupported_(true), bIpv6StateInOS_(true), isFirewallBlocked_(false),
    isDockedToTray_(false), isExternalConfigMode_(false)
{
    availableLanguageCodes_ << "en" ; // << "en_nsfw" << "ru" << "ar" << "es" << "fr" << "hu" << "it" << "ja" << "ko" << "nl" <<
                            // "zh" << "de" << "pl" << "tr" << "cs" << "da" << "el" << "pt" << "sk" << "th" << "vi" << "sv" <<
                            // "id" << "hi" << "hr";
}

QString PreferencesHelper::buildVersion()
{
    return AppVersion::instance().fullVersionString();
}

QList<QPair<QString, QString> > PreferencesHelper::availableLanguages() const
{
    QList<QPair<QString, QString> > pairs;
    for (const QString &lang : availableLanguageCodes_)
    {
        pairs << QPair<QString, QString>(LanguagesUtil::convertCodeToNative(lang), lang);
    }

    return pairs;
}

void PreferencesHelper::setProxyGatewayAddress(const QString &address)
{
    if (address != proxyGatewayAddress_)
    {
        proxyGatewayAddress_ = address;
        emit proxyGatewayAddressChanged(proxyGatewayAddress_);
    }
}

QString PreferencesHelper::getProxyGatewayAddress() const
{
    return proxyGatewayAddress_;
}

void PreferencesHelper::setAvailableOpenVpnVersions(const QStringList &list)
{
    if (list != availableOpenVpnVersions_)
    {
        availableOpenVpnVersions_ = list;
        emit availableOpenVpnVersionsChanged(availableOpenVpnVersions_);
    }
}

QStringList PreferencesHelper::getAvailableOpenVpnVersions()
{
    return availableOpenVpnVersions_;
}

QVector<ProtoTypes::TapAdapterType> PreferencesHelper::getAvailableTapAdapters(const QString & /*openVpnVersion*/)
{
    return QVector<ProtoTypes::TapAdapterType>() << ProtoTypes::TAP_ADAPTER << ProtoTypes::WINTUN_ADAPTER;
}

void PreferencesHelper::setPortMap(const ProtoTypes::ArrayPortMap &arr)
{
    portMap_ = arr;
    emit portMapChanged();
}

QVector<ProtoTypes::Protocol> PreferencesHelper::getAvailableProtocols()
{
    QVector<ProtoTypes::Protocol> p;
    for (int i = 0; i < portMap_.port_map_item_size(); ++i)
    {
#if defined(Q_OS_LINUX)
        const auto protocol = portMap_.port_map_item(i).protocol();
        if (protocol == ProtoTypes::Protocol::PROTOCOL_IKEV2) {
            continue;
        }
#elif defined(Q_OS_WINDOWS)
        const auto protocol = portMap_.port_map_item(i).protocol();
        if ((protocol == ProtoTypes::Protocol::PROTOCOL_WSTUNNEL) && !WinUtils::isWindows64Bit()) {
            continue;
        }
#endif
        p << portMap_.port_map_item(i).protocol();
    }
    return p;
}

QVector<uint> PreferencesHelper::getAvailablePortsForProtocol(ProtoTypes::Protocol protocol)
{
    QVector<uint> v;
    for (int i = 0; i < portMap_.port_map_item_size(); ++i)
    {
        if (portMap_.port_map_item(i).protocol() == protocol)
        {
            for (int p = 0; p < portMap_.port_map_item(i).ports_size(); ++p)
            {
                v << portMap_.port_map_item(i).ports(p);
            }
        }
    }

    return v;
}

void PreferencesHelper::setWifiSharingSupported(bool bSupported)
{
    if (bSupported != isWifiSharingSupported_)
    {
        isWifiSharingSupported_ = bSupported;
        emit wifiSharingSupportedChanged(isWifiSharingSupported_);
    }
}

bool PreferencesHelper::isWifiSharingSupported() const
{
    return isWifiSharingSupported_;
}

void PreferencesHelper::setBlockFirewall(bool b)
{
    if (isFirewallBlocked_ != b)
    {
        isFirewallBlocked_ = b;
        emit isFirewallBlockedChanged(isFirewallBlocked_);
    }
}

bool PreferencesHelper::isFirewallBlocked() const
{
    return isFirewallBlocked_;
}

void PreferencesHelper::setIsDockedToTray(bool b)
{
    if (isDockedToTray_ != b)
    {
        isDockedToTray_ = b;
        emit isDockedModeChanged(isDockedToTray_);
    }
}

bool PreferencesHelper::isDockedToTray() const
{
    return isDockedToTray_;
}

void PreferencesHelper::setIsExternalConfigMode(bool b)
{
    if (isExternalConfigMode_ != b)
    {
        isExternalConfigMode_ = b;
        emit isExternalConfigModeChanged(isExternalConfigMode_);
    }
}

bool PreferencesHelper::isExternalConfigMode() const
{
    return isExternalConfigMode_;
}

#ifdef Q_OS_WIN
void PreferencesHelper::setIpv6StateInOS(bool bEnabled)
{
    if (bIpv6StateInOS_ != bEnabled)
    {
        bIpv6StateInOS_ = bEnabled;
        emit ipv6StateInOSChanged(bIpv6StateInOS_);
    }
}

bool PreferencesHelper::getIpv6StateInOS() const
{
    return bIpv6StateInOS_;
}
#endif
