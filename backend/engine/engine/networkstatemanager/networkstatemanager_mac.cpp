#include "networkstatemanager_mac.h"

#include "../networkstatemanager/reachabilityevents.h"

NetworkStateManager_mac::NetworkStateManager_mac(QObject *parent) : INetworkStateManager(parent), bLastNetworkInterfaceInitialized_(false)
{
    connect(&ReachAbilityEvents::instance(), SIGNAL(networkStateChanged()), SLOT(onNetworkStateChanged()));
}

NetworkStateManager_mac::~NetworkStateManager_mac()
{
}

bool NetworkStateManager_mac::isOnline()
{
    QMutexLocker locker(&mutex_);
    QString strNetworkInterface;
    bool b = checkOnline(strNetworkInterface);

    if (!bLastNetworkInterfaceInitialized_)
    {
        lastNetworkInterface_ = strNetworkInterface;
        bLastNetworkInterfaceInitialized_ = true;
    }
    return b;
}

void NetworkStateManager_mac::onNetworkStateChanged()
{
    QMutexLocker locker(&mutex_);
    QString strNetworkInterface;
    bool b = checkOnline(strNetworkInterface);

    if (bLastNetworkInterfaceInitialized_)
    {
        if (strNetworkInterface != lastNetworkInterface_)
        {
            lastNetworkInterface_ = strNetworkInterface;
            emit stateChanged(b, strNetworkInterface);
        }
    }
    else
    {
        lastNetworkInterface_ = strNetworkInterface;
        bLastNetworkInterfaceInitialized_ = true;
        emit stateChanged(b, strNetworkInterface);
    }
}

bool NetworkStateManager_mac::checkOnline(QString &networkInterface)
{
    QString strReply;
    FILE *file = popen("echo 'show State:/Network/Global/IPv4' | scutil | grep PrimaryInterface | sed -e 's/.*PrimaryInterface : //'", "r");
    if (file)
    {
        char szLine[4096];
        while(fgets(szLine, sizeof(szLine), file) != 0)
        {
            strReply += szLine;
        }
        pclose(file);
    }

    strReply = strReply.trimmed();
    networkInterface = strReply;
    if (strReply.isEmpty())
    {
        return false;
    }
    else
    {
        return true;
    }
}

