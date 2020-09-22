#ifndef LOCATIONSMODEL_LOCATIONNODE_H
#define LOCATIONSMODEL_LOCATIONNODE_H

#include <QObject>
#include <QVector>

#include "engine/types/locationid.h"
#include "engine/apiinfo/node.h"
#include "engine/apiinfo/staticips.h"



namespace locationsmodel {

class BaseNode
{
public:
    explicit BaseNode() {}
    virtual ~BaseNode() {}


    virtual QString getIp(int ind) const { Q_ASSERT(false); return ""; }
    virtual QString getHostname() const { Q_ASSERT(false); return ""; }

    virtual QString getCustomOvpnConfigPath() const { Q_ASSERT(false); return ""; }

    virtual QString getStaticIpDnsName() const { Q_ASSERT(false); return ""; }
    virtual QString getStaticIpUsername() const { Q_ASSERT(false); return ""; }
    virtual QString getStaticIpPassword() const { Q_ASSERT(false); return ""; }
    virtual apiinfo::StaticIpPortsVector getStaticIpPorts() const { Q_ASSERT(false); return apiinfo::StaticIpPortsVector(); }
};


class ApiLocationNode : public BaseNode
{
public:
    explicit ApiLocationNode(const QStringList &ips, const QString &hostname) : hostname_(hostname)
    {
        Q_ASSERT(ips.count() == 3);
        ips_[0] = ips[0];
        ips_[1] = ips[1];
        ips_[2] = ips[2];
    }
    virtual ~ApiLocationNode() {}

    QString getIp(int ind) const override
    {
        Q_ASSERT(ind >= 0 && ind < 3);
        return ips_[ind];
    }
    QString getHostname() const override
    {
        return hostname_;
    }


private:
    QString ips_[3];
    QString hostname_;
};


class StaticLocationNode : public BaseNode
{
public:
    explicit StaticLocationNode();
    virtual ~StaticLocationNode();
};

class CustomConfigLocationNode : public BaseNode
{
public:
    explicit CustomConfigLocationNode();
    virtual ~CustomConfigLocationNode();
};



} //namespace locationsmodel

#endif // LOCATIONSMODEL_LOCATIONNODE_H
