#include "dnsresolutionsettings.h"
#include "utils/logger.h"

namespace types {

DnsResolutionSettings::DnsResolutionSettings() : bAutomatic_(true)
{
}

void DnsResolutionSettings::set(bool bAutomatic, const QString &manualIp)
{
    bAutomatic_ = bAutomatic;
    manualIp_ = manualIp;
}

void DnsResolutionSettings::debugToLog()
{
    QString manualIp = manualIp_;
    if (bAutomatic_)
    {
        qCDebug(LOG_BASIC) << "DNS mode: automatic";
    }
    else if (manualIp.trimmed().isEmpty())
    {
        qCDebug(LOG_BASIC) << "DNS mode: automatic (empty ip)";
    }
    else
    {
        qCDebug(LOG_BASIC) << "DNS mode: manual";
        qCDebug(LOG_BASIC) << "DNS ip:" << manualIp_;
    }
}

QJsonObject DnsResolutionSettings::toJsonObject() const
{
    QJsonObject json;
    json["isAutomatic"] = bAutomatic_;
    json["manualIp"] = manualIp_;
    return json;
}

bool DnsResolutionSettings::fromJsonObject(const QJsonObject &json)
{
    if (json.contains("isAutomatic")) bAutomatic_ = json["isAutomatic"].toBool(true);
    if (json.contains("manualIp")) manualIp_ = json["manualIp"].toString();
    return true;
}

bool DnsResolutionSettings::getIsAutomatic() const
{
    QString manualIp = manualIp_;
    return bAutomatic_ || manualIp.trimmed().isEmpty();
}

void DnsResolutionSettings::setIsAutomatic(bool bAutomatic)
{
    bAutomatic_ = bAutomatic;
}

QString DnsResolutionSettings::getManualIp() const
{
    return manualIp_;
}

void DnsResolutionSettings::setManualIp(const QString &manualIp)
{
    manualIp_ = manualIp;
}

} //namespace types
