#pragma once

#include "engine/helper/helper.h"
#include <QMap>

class Ipv6Controller_mac
{
public:
    static Ipv6Controller_mac &instance()
    {
        static Ipv6Controller_mac i;
        return i;
    }

    void setHelper(Helper *helper);
    void disableIpv6();
    void restoreIpv6();

private:
    Ipv6Controller_mac();
    ~Ipv6Controller_mac();

    bool bIsDisabled_;
    Helper *helper_;


    QMap<QString, QString> ipv6States_;

    void saveIpv6States(const QStringList &networkIntefaces);
    bool isEnabledIPv6(const QString &interface);
    QString getStateFromSaved(const QString &interface);
    QString getStateFromCmd(const QString &interface);

};
