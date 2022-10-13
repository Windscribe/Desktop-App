#include "apiresolutionsettings.h"
#include "utils/logger.h"

namespace types {

ApiResolutionSettings::ApiResolutionSettings() : bAutomatic_(true)
{
}

void ApiResolutionSettings::set(bool bAutomatic, const QString &manualIp)
{
    bAutomatic_ = bAutomatic;
    manualIp_ = manualIp;
}

bool ApiResolutionSettings::getIsAutomatic() const
{
    return bAutomatic_;
}

void ApiResolutionSettings::setIsAutomatic(bool bAutomatic)
{
    bAutomatic_ = bAutomatic;
}

QString ApiResolutionSettings::getManualIp() const
{
    return manualIp_;
}

void ApiResolutionSettings::setManualIp(const QString &manualIp)
{
    manualIp_ = manualIp;
}

QDataStream& operator <<(QDataStream &stream, const ApiResolutionSettings &o)
{
    stream << o.versionForSerialization_;
    stream << o.bAutomatic_ << o.manualIp_;
    return stream;

}
QDataStream& operator >>(QDataStream &stream, ApiResolutionSettings &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_) {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    stream >> o.bAutomatic_ >> o.manualIp_;
    return stream;

}

QDebug operator<<(QDebug dbg, const ApiResolutionSettings &ds)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{isAutomatic:" << ds.bAutomatic_ << "; ";
    dbg << "manualIp:" << ds.manualIp_ << "}";
    return dbg;
}

} //namespace types
