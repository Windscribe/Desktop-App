#ifndef DNSRESOLUTIONSETTINGS_H
#define DNSRESOLUTIONSETTINGS_H

#include <QString>
#include "ipc/generated_proto/types.pb.h"

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
    bool bAutomatic_;
    QString manualIp_;
};

#endif // DNSRESOLUTIONSETTINGS_H
