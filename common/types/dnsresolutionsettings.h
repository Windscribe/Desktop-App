#ifndef TYPES_DNSRESOLUTIONSETTINGS_H
#define TYPES_DNSRESOLUTIONSETTINGS_H

#include <QDataStream>
#include <QJsonObject>
#include <QString>

namespace types {

class DnsResolutionSettings
{
public:
    explicit DnsResolutionSettings();

    void set(bool bAutomatic, const QString &manualIp);
    bool getIsAutomatic() const;
    void setIsAutomatic(bool bAutomatic);
    QString getManualIp() const;
    void setManualIp(const QString &manualIp);
    void debugToLog();

    QJsonObject toJsonObject() const;
    bool fromJsonObject(const QJsonObject &json);

    bool operator==(const DnsResolutionSettings &other) const
    {
        return other.bAutomatic_ == bAutomatic_ &&
               other.manualIp_ == manualIp_;
    }

    bool operator!=(const DnsResolutionSettings &other) const
    {
        return !(*this == other);
    }

private:
    bool bAutomatic_;
    QString manualIp_;
};

} //namespace types

#endif // TYPES_DNSRESOLUTIONSETTINGS_H
