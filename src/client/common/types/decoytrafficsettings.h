#pragma once

#include <QDataStream>
#include <QJsonObject>
#include <QSettings>

namespace types {

enum class DecoyTrafficVolume { kLow = 0, kMedium, kHigh };

QString decoyTrafficVolumeToString(DecoyTrafficVolume v);
DecoyTrafficVolume decoyTrafficVolumeFromString(const QString &s);

class DecoyTrafficSettings
{
public:
    explicit DecoyTrafficSettings();
    DecoyTrafficSettings(const QJsonObject& json);

    void setEnabled(bool bEnabled);
    bool isEnabled() const;
    void setVolume(DecoyTrafficVolume volume);
    DecoyTrafficVolume volume() const;

    bool operator==(const DecoyTrafficSettings &other) const
    {
        return other.bEnabled_ == bEnabled_ &&
               other.volume_ == volume_;
    }

    bool operator!=(const DecoyTrafficSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const;
    void fromIni(const QSettings &settings);
    void toIni(QSettings &settings) const;

    friend QDataStream& operator <<(QDataStream &stream, const DecoyTrafficSettings &o);
    friend QDataStream& operator >>(QDataStream &stream, DecoyTrafficSettings &o);

private:
    bool bEnabled_ = false;
    DecoyTrafficVolume volume_ = DecoyTrafficVolume::kLow;

    static const inline QString kIniIsEnabledProp = "DecoyTrafficEnabled";
    static const inline QString kIniVolumeProp = "DecoyTrafficVolume";
    static const inline QString kJsonIsEnabledProp = "isEnabled";
    static const inline QString kJsonVolumeProp = "volume";
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types
