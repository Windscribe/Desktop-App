#include "keepalivemanager.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/dnsresolver/dnsresolver.h"
#include "engine/hardcodedsettings.h"

KeepAliveManager::KeepAliveManager(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    isEnabled_(false), curConnectState_(CONNECT_STATE_DISCONNECTED), pingHostIcmp_(this, stateController)
{
    connect(&DnsResolver::instance(), SIGNAL(resolved(QString,QHostInfo,void *)), SLOT(onDnsResolvedFinished(QString, QHostInfo,void *)));
    connect(stateController, SIGNAL(stateChanged(CONNECT_STATE,DISCONNECT_REASON,CONNECTION_ERROR,LocationID)), SLOT(onConnectStateChanged(CONNECT_STATE,DISCONNECT_REASON,CONNECTION_ERROR,LocationID)));
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));
    connect(&pingHostIcmp_, SIGNAL(pingFinished(bool,int,QString,bool)), SLOT(onPingFinished(bool,int,QString,bool)));
}

void KeepAliveManager::setEnabled(bool isEnabled)
{
    isEnabled_ = isEnabled;
    if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_)
    {
        DnsResolver::instance().lookup(HardcodedSettings::instance().serverUrl(), this);
    }
    else
    {
        timer_.stop();
    }
}

void KeepAliveManager::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECTION_ERROR err, const LocationID &location)
{
    Q_UNUSED(reason);
    Q_UNUSED(err);
    Q_UNUSED(location);

    curConnectState_ = state;
    if (state == CONNECT_STATE_CONNECTED && isEnabled_)
    {
        DnsResolver::instance().lookup(HardcodedSettings::instance().serverUrl(), this);
    }
    else
    {
        timer_.stop();
    }
}

void KeepAliveManager::onTimer()
{
    for (int i = 0; i < ips_.count(); ++i)
    {
        if (!ips_[i].bFailed_)
        {
            pingHostIcmp_.addHostForPing(ips_[i].ip_);
            return;
        }
    }
    // if all failed, then select random
    if (ips_.count() > 0)
    {
        int ind = Utils::generateIntegerRandom(0, ips_.count() - 1);
        pingHostIcmp_.addHostForPing(ips_[ind].ip_);
    }
}

void KeepAliveManager::onDnsResolvedFinished(const QString &hostname, const QHostInfo &hostInfo, void *userPointer)
{
    Q_UNUSED(hostname);

    if (userPointer == this)
    {
        if (hostInfo.error() == QHostInfo::NoError && hostInfo.addresses().count() > 0)
        {
            ips_.clear();
            for (int i = 0; i < hostInfo.addresses().count(); ++i)
            {
                ips_ << IP_DESCR(hostInfo.addresses()[i].toString());
            }

            if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_)
            {
                timer_.start(KEEP_ALIVE_TIMEOUT);
            }
        }
        else
        {
            if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_)
            {
                DnsResolver::instance().lookup(HardcodedSettings::instance().serverUrl(), this);
            }
        }
    }
}

void KeepAliveManager::onPingFinished(bool bSuccess, int timems, const QString &ip, bool isFromDisconnectedState)
{
    Q_UNUSED(timems);
    Q_UNUSED(isFromDisconnectedState);

    for (int i = 0; i < ips_.count(); ++i)
    {
        if (ips_[i].ip_ == ip)
        {
            ips_[i].bFailed_ = !bSuccess;
            break;
        }
    }
}
