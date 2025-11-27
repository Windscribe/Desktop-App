#include "preferenceshelper.h"
#include "utils/languagesutil.h"
#include "version/appversion.h"

PreferencesHelper::PreferencesHelper(QObject *parent) : QObject(parent),
    isWifiSharingSupported_(true), isDockedToTray_(false), isExternalConfigMode_(false)
{
    availableLanguageCodes_ << "ar" << "cs" << "de" << "el" << "en" << "es" << "fa" << "fr" << "hi" << "id" << "it" << "ja" << "ko"
        << "pl" << "pt" << "ru" << "sk" << "tr" << "uk" << "vi" << "zh-CN" << "zh-TW";
}

QString PreferencesHelper::buildVersion()
{
    return AppVersion::instance().fullVersionString();
}

QList<QPair<QString, QVariant>> PreferencesHelper::availableLanguages() const
{
    QList<QPair<QString, QVariant>> pairs;
    for (const QString &lang : availableLanguageCodes_) {
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

QVector<TAP_ADAPTER_TYPE> PreferencesHelper::getAvailableTapAdapters(const QString & /*openVpnVersion*/)
{
    return QVector<TAP_ADAPTER_TYPE>() << DCO_ADAPTER << WINTUN_ADAPTER;
}

void PreferencesHelper::setPortMap(const api_responses::PortMap &portMap)
{
    portMap_ = portMap;
    emit portMapChanged();
}

QVector<types::Protocol> PreferencesHelper::getAvailableProtocols()
{
    QVector<types::Protocol> p;
    for (const auto &it : portMap_.const_items())
        p << it.protocol;
    return p;
}

QVector<uint> PreferencesHelper::getAvailablePortsForProtocol(types::Protocol protocol)
{
    QVector<uint> v;
    for (const auto &it : portMap_.const_items())
    {
        if (it.protocol == protocol)
        {
            for (int p = 0; p < it.ports.size(); ++p)
            {
                v << it.ports[p];
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

void PreferencesHelper::setCurrentProtocol(const types::Protocol &protocol)
{
    if (currentProtocol_ != protocol) {
        currentProtocol_ = protocol;
        emit currentProtocolChanged(currentProtocol_);
    }
}

types::Protocol PreferencesHelper::currentProtocol() const
{
    return currentProtocol_;
}