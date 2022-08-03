#ifndef TYPES_DNSRESOLUTIONSETTINGS_H
#define TYPES_DNSRESOLUTIONSETTINGS_H

#include <QDataStream>
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

    bool operator==(const DnsResolutionSettings &other) const
    {
        return other.bAutomatic_ == bAutomatic_ &&
               other.manualIp_ == manualIp_;
    }

    bool operator!=(const DnsResolutionSettings &other) const
    {
        return !(*this == other);
    }

    friend QDataStream& operator <<(QDataStream &stream, const DnsResolutionSettings &o)
    {
        stream << versionForSerialization_;
        stream << o.bAutomatic_ << o.manualIp_;
        return stream;
    }
    friend QDataStream& operator >>(QDataStream &stream, DnsResolutionSettings &o)
    {
        quint32 version;
        stream >> version;
        Q_ASSERT(version == versionForSerialization_);
        if (version > versionForSerialization_)
        {
            return stream;
        }
        stream >> o.bAutomatic_ >> o.manualIp_;
        return stream;
    }


private:
    bool bAutomatic_;
    QString manualIp_;
    static constexpr quint32 versionForSerialization_ = 1;
};

} //namespace types

#endif // TYPES_DNSRESOLUTIONSETTINGS_H
