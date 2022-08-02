#include "dnsresolutionsettings.h"
#include "utils/logger.h"

namespace types {

DnsResolutionSettings::DnsResolutionSettings() : bAutomatic_(true)
{
}

DnsResolutionSettings::DnsResolutionSettings(const ProtoTypes::ApiResolution &d)
{
    bAutomatic_ = d.is_automatic();
    manualIp_ = QString::fromStdString(d.manual_ip());
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

ProtoTypes::ApiResolution DnsResolutionSettings::convertToProtobuf() const
{
    ProtoTypes::ApiResolution drs;
    drs.set_is_automatic(bAutomatic_);
    drs.set_manual_ip(manualIp_.toStdString());
    return drs;
}

bool DnsResolutionSettings::isEqual(const DnsResolutionSettings &other) const
{
    return bAutomatic_ == other.bAutomatic_ && manualIp_ == other.manualIp_;
}

bool DnsResolutionSettings::getIsAutomatic() const
{
    QString manualIp = manualIp_;
    return bAutomatic_ || manualIp.trimmed().isEmpty();
}

QString DnsResolutionSettings::getManualIp() const
{
    return manualIp_;
}

} //namespace types
