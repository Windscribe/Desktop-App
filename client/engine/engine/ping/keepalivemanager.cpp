#include "keepalivemanager.h"

#include "engine/dnsresolver/dnsrequest.h"
#include "engine/dnsresolver/dnsserversconfiguration.h"
#include "engine/connectstatecontroller/iconnectstatecontroller.h"
#include "utils/hardcodedsettings.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "utils/ws_assert.h"

KeepAliveManager::KeepAliveManager(QObject *parent, IConnectStateController *stateController, NetworkAccessManager *networkAccessManager) : QObject(parent),
    isEnabled_(false), curConnectState_(CONNECT_STATE_DISCONNECTED), pingHosts_(this, stateController, networkAccessManager)
{
    connect(stateController, &IConnectStateController::stateChanged, this, &KeepAliveManager::onConnectStateChanged);
    connect(&timer_, &QTimer::timeout, this, &KeepAliveManager::onTimer);
    connect(&pingHosts_, &PingMultipleHosts::pingFinished, this, &KeepAliveManager::onPingFinished);
}

void KeepAliveManager::setEnabled(bool isEnabled)
{
    isEnabled_ = isEnabled;
    if (curConnectState_ == CONNECT_STATE_CONNECTED && isEnabled_)
    {
        DnsRequest *dnsRequest = new DnsRequest(this, HardcodedSettings::instance().serverUrl(), DnsServersConfiguration::instance().getCurrentDnsServers());
        connect(dnsRequest, &DnsRequest::finished, this, &KeepAliveManager::onDnsRequestFinished);
        dnsRequest->lookup();
    }
    else
    {
        timer_.stop();
    }
}

void KeepAliveManager::onConnectStateChanged(CONNECT_STATE state, DISCONNECT_REASON reason, CONNECT_ERROR err, const LocationID &location)
{
    Q_UNUSED(reason);
    Q_UNUSED(err);
    Q_UNUSED(location);

    curConnectState_ = state;
    if (state == CONNECT_STATE_CONNECTED && isEnabled_)
    {
        DnsRequest *dnsRequest = new DnsRequest(this, HardcodedSettings::instance().serverUrl(), DnsServersConfiguration::instance().getCurrentDnsServers());
        connect(dnsRequest, &DnsRequest::finished, this, &KeepAliveManager::onDnsRequestFinished);
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
            pingHosts_.addHostForPing(ips_[i].ip_, QString(), PingType::kIcmp);
            return;
        }
    }
    // if all failed then select random
    if (ips_.count() > 0)
    {
        int ind = Utils::generateIntegerRandom(0, ips_.count() - 1);
        pingHosts_.addHostForPing(ips_[ind].ip_, QString(), PingType::kIcmp);
    }
}

void KeepAliveManager::onDnsRequestFinished()
{
    DnsRequest *dnsRequest = qobject_cast<DnsRequest *>(sender());
    WS_ASSERT(dnsRequest != nullptr);

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
            DnsRequest *request = new DnsRequest(this, HardcodedSettings::instance().serverUrl(), DnsServersConfiguration::instance().getCurrentDnsServers());
            connect(request, &DnsRequest::finished, this, &KeepAliveManager::onDnsRequestFinished);
            request->lookup();
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
