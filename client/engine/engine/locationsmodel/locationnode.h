#ifndef LOCATIONSMODEL_LOCATIONNODE_H
#define LOCATIONSMODEL_LOCATIONNODE_H

#include <QObject>
#include <QVector>
#include "engine/apiinfo/node.h"
#include "engine/apiinfo/staticips.h"
#include "utils/ws_assert.h"

namespace locationsmodel {

class BaseNode
{
public:
    explicit BaseNode() {}
    virtual ~BaseNode() {}


    virtual QString getIp(int /*ind*/) const { WS_ASSERT(false); return ""; }
    virtual QString getHostname() const { WS_ASSERT(false); return ""; }
    virtual int getWeight() const {  WS_ASSERT(false); return 1; }

    virtual QString getWgPubKey() const { WS_ASSERT(false); return ""; }
    virtual QString getWgIp() const { WS_ASSERT(false); return ""; }

    virtual QString getStaticIpDnsName() const { WS_ASSERT(false); return ""; }
    virtual QString getStaticIpUsername() const { WS_ASSERT(false); return ""; }
    virtual QString getStaticIpPassword() const { WS_ASSERT(false); return ""; }
    virtual apiinfo::StaticIpPortsVector getStaticIpPorts() const { WS_ASSERT(false); return apiinfo::StaticIpPortsVector(); }
};


class ApiLocationNode : public BaseNode
{
public:
    explicit ApiLocationNode(const QStringList &ips, const QString &hostname, int weight, const QString &wg_pubkey)
        : hostname_(hostname), wg_pubkey_(wg_pubkey), weight_(weight)
    {
        WS_ASSERT(ips.count() == 3);
        ips_[0] = ips[0];
        ips_[1] = ips[1];
        ips_[2] = ips[2];
    }
    virtual ~ApiLocationNode() {}

    QString getIp(int ind) const override
    {
        WS_ASSERT(ind >= 0 && ind < 3);
        return ips_[ind];
    }
    QString getHostname() const override
    {
        return hostname_;
    }

    QString getWgPubKey() const override
    {
        return wg_pubkey_;
    }

    int getWeight() const override {  return weight_; }


private:
    QString ips_[3];
    QString hostname_;
    QString wg_pubkey_;
    int weight_;
};


class StaticLocationNode : public ApiLocationNode
{
public:
    explicit StaticLocationNode(const QStringList &ips, const QString &hostname, const QString &wg_pubkey, const QString &wg_ip, const QString &dnsName,
                                const QString &username, const QString &password, const apiinfo::StaticIpPortsVector &ipPortsVector) :
            ApiLocationNode(ips, hostname, 1, wg_pubkey), dnsName_(dnsName), username_(username), password_(password), wg_ip_(wg_ip), ipPortsVector_(ipPortsVector) {}
    virtual ~StaticLocationNode() {}

    QString getStaticIpDnsName() const override { return dnsName_; }
    QString getStaticIpUsername() const override { return username_; }
    QString getStaticIpPassword() const override { return password_; }
    apiinfo::StaticIpPortsVector getStaticIpPorts() const override { return ipPortsVector_; }
    QString getWgIp() const override { return wg_ip_; }

private:
    QString dnsName_;
    QString username_;
    QString password_;
    QString wg_ip_;
    apiinfo::StaticIpPortsVector ipPortsVector_;
};



} //namespace locationsmodel

#endif // LOCATIONSMODEL_LOCATIONNODE_H
