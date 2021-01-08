#ifndef DNSRESOLUTIONSETTINGS_H
#define DNSRESOLUTIONSETTINGS_H

#include <QString>
#include "utils/protobuf_includes.h"

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

private:
    bool isInitialized_;
    bool isEmptyManualIp_;
    bool bAutomatic_;
    QString manualIp_;
};

#endif // DNSRESOLUTIONSETTINGS_H
