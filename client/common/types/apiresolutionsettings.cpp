#include "apiresolutionsettings.h"
#include "types/enums.h"
#include "utils/ipvalidation.h"
#include "utils/log/logger.h"

namespace types {

ApiResolutionSettings::ApiResolutionSettings()
{
}

void ApiResolutionSettings::set(bool bAutomatic, const QString &manualAddress)
{
    bAutomatic_ = bAutomatic;
    manualAddress_ = manualAddress;
}

bool ApiResolutionSettings::getIsAutomatic() const
{
    return bAutomatic_;
}

void ApiResolutionSettings::setIsAutomatic(bool bAutomatic)
{
    bAutomatic_ = bAutomatic;
}

QString ApiResolutionSettings::getManualAddress() const
{
    return manualAddress_;
}

void ApiResolutionSettings::setManualAddress(const QString &manualAddress)
{
    manualAddress_ = manualAddress;
}

QDataStream& operator <<(QDataStream &stream, const ApiResolutionSettings &o)
{
    stream << o.versionForSerialization_;
    stream << o.bAutomatic_ << o.manualAddress_;
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
    stream >> o.bAutomatic_ >> o.manualAddress_;
    return stream;
}

} //namespace types
