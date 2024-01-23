#include "apiresolutionsettings.h"
#include "utils/logger.h"

namespace types {

ApiResolutionSettings::ApiResolutionSettings() : bAutomatic_(true)
{
}

ApiResolutionSettings::ApiResolutionSettings(const QJsonObject &json)
{
    if (json.contains(jsonInfo_.kIsAutomaticProp) && json[jsonInfo_.kIsAutomaticProp].isBool())
        bAutomatic_ = json[jsonInfo_.kIsAutomaticProp].toBool();

    if (json.contains(jsonInfo_.kManualAddressProp) && json[jsonInfo_.kManualAddressProp].isString())
        manualAddress_ = json[jsonInfo_.kManualAddressProp].toString();
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

QDebug operator<<(QDebug dbg, const ApiResolutionSettings &ds)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    dbg << "{isAutomatic:" << ds.bAutomatic_ << "; ";
    dbg << "manualAddress:" << ds.manualAddress_ << "}";
    return dbg;
}

} //namespace types
