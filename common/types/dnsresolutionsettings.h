#ifndef TYPES_DNSRESOLUTIONSETTINGS_H
#define TYPES_DNSRESOLUTIONSETTINGS_H

#include <QString>
#include "utils/protobuf_includes.h"

namespace types {

class DnsResolutionSettings
{
public:
    DnsResolutionSettings();
    explicit DnsResolutionSettings(const ProtoTypes::ApiResolution &d);

    void set(bool bAutomatic, const QString &manualIp);

    void debugToLog();
    ProtoTypes::ApiResolution convertToProtobuf() const;

    bool isEqual(const DnsResolutionSettings &other) const;

    bool getIsAutomatic() const;
    QString getManualIp() const;

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
