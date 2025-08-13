#pragma once

#include <QDataStream>
#include <QJsonObject>
#include <QSettings>
#include <QString>

namespace types {

class ApiResolutionSettings
{
public:
    explicit ApiResolutionSettings();

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

    friend QDataStream& operator <<(QDataStream &stream, const ApiResolutionSettings &o);
    friend QDataStream& operator >>(QDataStream &stream, ApiResolutionSettings &o);

private:
    bool bAutomatic_ = true;
    QString manualAddress_;

    static const inline QString kIniIsAutomaticProp = "APIResolutionMode";
    static const inline QString kIniManualAddressProp = "APIResolutionAddress";
    static const inline QString kJsonIsAutomaticProp = "isAutomatic";
    static const inline QString kJsonManualAddressProp = "manualAddress";
    static constexpr quint32 versionForSerialization_ = 1;  // should increment the version if the data format is changed
};

} //namespace types

