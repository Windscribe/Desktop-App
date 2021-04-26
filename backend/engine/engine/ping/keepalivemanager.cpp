#include "keepalivemanager.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "engine/dnsresolver/dnsrequest.h"
#include "engine/hardcodedsettings.h"

KeepAliveManager::KeepAliveManager(QObject *parent, IConnectStateController *stateController) : QObject(parent),
    isEnabled_(false), curConnectState_(CONNECT_STATE_DISCONNECTED), pingHostIcmp_(this, stateController)
{
    connect(stateController, SIGNAL(stateChanged(CONNECT_STATE,DISCONNECT_REASON,CONNECTION_ERROR,LocationID)), SLOT(onConnectStateChanged(CONNECT_STATE,DISCONNECT_REASON,CONNECTION_ERROR,LocationID)));
    connect(&timer_, SIGNAL(timeout()), SLOT(onTimer()));
    connect(&pingHostIcmp_, SIGNAL(pingFinished(bool,int,QString,bool)), SLOT(onPingFinished(bool,int,QString,bool)));
}

void KeepAliveManager::setEnabled(bool isEnabled)
{
    isEnabled_ = isEnabled;
    if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_)
    {
        DnsRequest *dnsRequest = new DnsRequest(this, HardcodedSettings::instance().serverUrl());
        connect(dnsRequest, SIGNAL(finished()), SLOT(onDnsRequestFinished()));
        dnsRequest->lookup();
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
        DnsRequest *dnsRequest = new DnsRequest(this, HardcodedSettings::instance().serverUrl());
        connect(dnsRequest, SIGNAL(finished()), SLOT(onDnsRequestFinished()));
        dnsRequest->lookup();
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

void KeepAliveManager::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    Q_ASSERT(dnsRequest != nullptr);

    if (!dnsRequest->isError())
    {
        ips_.clear();
        for (const QString &ip : dnsRequest->ips())
        {
            ips_ << IP_DESCR(ip);
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
            DnsRequest *dnsRequest = new DnsRequest(this, HardcodedSettings::instance().serverUrl());
            connect(dnsRequest, SIGNAL(finished()), SLOT(onDnsRequestFinished()));
            dnsRequest->lookup();
        }
    }
    dnsRequest->deleteLater();
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
