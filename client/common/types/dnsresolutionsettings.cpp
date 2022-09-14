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

bool DnsResolutionSettings::getIsAutomatic() const
{
    return bAutomatic_;
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

QDataStream& operator <<(QDataStream &stream, const DnsResolutionSettings &o)
{
    stream << o.versionForSerialization_;
    stream << o.bAutomatic_ << o.manualIp_;
    return stream;

}
QDataStream& operator >>(QDataStream &stream, DnsResolutionSettings &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_)
    {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.bAutomatic_ >> o.manualIp_;
    return stream;

}

QDebug operator<<(QDebug dbg, const DnsResolutionSettings &ds)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{isAutomatic:" << ds.bAutomatic_ << "; ";
    dbg << "manualIp:" << ds.manualIp_ << "}";
    return dbg;
}

} //namespace types
