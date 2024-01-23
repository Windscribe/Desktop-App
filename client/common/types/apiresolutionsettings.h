#pragma once

#include <QDataStream>
#include <QJsonObject>
#include <QString>

namespace types {

class ApiResolutionSettings
{
public:
    struct JsonInfo
    {
        JsonInfo& operator=(const JsonInfo&) { return *this; }

        const QString kIsAutomaticProp = "isAutomatic";
        const QString kManualAddressProp = "manualAddress";
    };

    explicit ApiResolutionSettings();
    ApiResolutionSettings(const QJsonObject& json);

    void set(bool bAutomatic, const QString &manualAddress);
    bool getIsAutomatic() const;
    void setIsAutomatic(bool bAutomatic);
    QString getManualAddress() const;
    void setManualAddress(const QString &manualAddress);

    bool operator==(const ApiResolutionSettings &other) const
    {
        return other.bAutomatic_ == bAutomatic_ &&
               other.manualAddress_ == manualAddress_;
    }

    bool operator!=(const ApiResolutionSettings &other) const
    {
        return !(*this == other);
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json[jsonInfo_.kIsAutomaticProp] = bAutomatic_;
        json[jsonInfo_.kManualAddressProp] = manualAddress_;
        return json;
    }

    friend QDataStream& operator <<(QDataStream &stream, const ApiResolutionSettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ApiResolutionSettings &o);

    friend QDebug operator<<(QDebug dbg, const ApiResolutionSettings &ds);

private:
    bool bAutomatic_;
    QString manualAddress_;
    JsonInfo jsonInfo_;

    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types

