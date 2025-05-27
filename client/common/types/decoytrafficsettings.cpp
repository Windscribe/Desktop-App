#include "decoytrafficsettings.h"
#include "utils/ws_assert.h"

namespace types {

QString decoyTrafficVolumeToString(DecoyTrafficVolume v)
{
    if (v == DecoyTrafficVolume::kLow) return "Low";
    else if (v == DecoyTrafficVolume::kMedium) return "Medium";
    else if (v == DecoyTrafficVolume::kHigh) return "High";
    else { WS_ASSERT(false); return "Uknown"; }
}

DecoyTrafficVolume decoyTrafficVolumeFromString(const QString &s)
{
    if (s == "Low") return DecoyTrafficVolume::kLow;
    else if (s == "Medium") return DecoyTrafficVolume::kMedium;
    else if (s == "High") return DecoyTrafficVolume::kHigh;
    else return DecoyTrafficVolume::kLow;
}

DecoyTrafficSettings::DecoyTrafficSettings()
{
}

DecoyTrafficSettings::DecoyTrafficSettings(const QJsonObject &json)
{
    if (json.contains(kJsonIsEnabledProp) && json[kJsonIsEnabledProp].isBool()) {
        bEnabled_ = json[kJsonIsEnabledProp].toBool();
    }

    if (json.contains(kJsonVolumeProp) && json[kJsonVolumeProp].isDouble()) {
        int val = json[kJsonVolumeProp].toInt();
        if (val >= 0 && val <= 2)
            volume_ = (DecoyTrafficVolume)val;
    }
}

void DecoyTrafficSettings::setEnabled(bool bEnabled)
{
    bEnabled_ = bEnabled;
}

bool DecoyTrafficSettings::isEnabled() const
{
    return bEnabled_;
}

void DecoyTrafficSettings::setVolume(DecoyTrafficVolume volume)
{
    volume_ = volume;
}

DecoyTrafficVolume DecoyTrafficSettings::volume() const
{
    return volume_;
}

QJsonObject DecoyTrafficSettings::toJson() const
{
    QJsonObject json;
    json[kJsonIsEnabledProp] = bEnabled_;
    json[kJsonVolumeProp] = (int)volume_;
    return json;
}

void DecoyTrafficSettings::fromIni(const QSettings &settings)
{
    bEnabled_ = settings.value(kIniIsEnabledProp, false).toBool();
    volume_ = decoyTrafficVolumeFromString(settings.value(kIniVolumeProp, decoyTrafficVolumeToString(DecoyTrafficVolume::kLow)).toString());
}

void DecoyTrafficSettings::toIni(QSettings &settings) const
{
    settings.setValue(kIniIsEnabledProp, bEnabled_);
    settings.setValue(kIniVolumeProp, decoyTrafficVolumeToString(volume_));
}

QDataStream& operator <<(QDataStream &stream, const DecoyTrafficSettings &o)
{
    stream << o.versionForSerialization_;
    stream << o.bEnabled_ << (int)o.volume_;
    return stream;

}
QDataStream& operator >>(QDataStream &stream, DecoyTrafficSettings &o)
{
    quint32 version;
    stream >> version;
    if (version > o.versionForSerialization_) {
        stream.setStatus(QDataStream::ReadCorruptData);
        return stream;
    }
    int vol;
    stream >> o.bEnabled_ >> vol;
    o.volume_ = (DecoyTrafficVolume)vol;
    return stream;
}

} //namespace types
