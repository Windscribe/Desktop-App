#include "apiresolutionsettings.h"
#include "types/enums.h"
#include "utils/ipvalidation.h"
#include "utils/logger.h"

namespace types {

ApiResolutionSettings::ApiResolutionSettings() : bAutomatic_(true)
{
}

ApiResolutionSettings::ApiResolutionSettings(const QJsonObject &json)
{
    if (json.contains(kJsonIsAutomaticProp) && json[kJsonIsAutomaticProp].isBool()) {
        bAutomatic_ = json[kJsonIsAutomaticProp].toBool();
    }

    if (json.contains(kJsonManualAddressProp) && json[kJsonManualAddressProp].isString()) {
        QString address = json[kJsonManualAddressProp].toString();
        if (IpValidation::isIp(address)) {
            manualAddress_ = address;
        }
    }
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

QJsonObject ApiResolutionSettings::toJson() const
{
    QJsonObject json;
    json[kJsonIsAutomaticProp] = bAutomatic_;
    json[kJsonManualAddressProp] = manualAddress_;
    return json;
}

void ApiResolutionSettings::fromIni(const QSettings &settings)
{
    QString prevMode = TOGGLE_MODE_toString(bAutomatic_ ? TOGGLE_MODE_AUTO : TOGGLE_MODE_MANUAL);
    TOGGLE_MODE mode = TOGGLE_MODE_fromString(settings.value(kIniIsAutomaticProp, prevMode).toString());
    bAutomatic_ = (mode == TOGGLE_MODE_AUTO);
    QString address = settings.value(kIniManualAddressProp, manualAddress_).toString();
    if (IpValidation::isIp(address)) {
        manualAddress_ = address;
    }
}

void ApiResolutionSettings::toIni(QSettings &settings) const
{
    settings.setValue(kIniIsAutomaticProp, TOGGLE_MODE_toString(bAutomatic_ ? TOGGLE_MODE_AUTO : TOGGLE_MODE_MANUAL));
    settings.setValue(kIniManualAddressProp, manualAddress_);
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
