#include "whitelistipsmanager.h"
#include "utils/logger.h"

WhitelistIpsManager::WhitelistIpsManager(QObject *parent) : QObject(parent)
{
}

void WhitelistIpsManager::add(const QString &hostname, const QStringList &ips)
{
//    if (!hash_.contains(hostname))
//        qCDebug(LOG_NETWORK) << "Whitelist ips added" << hostname << ips;
    hash_[hostname] = ips;
    updateIps();
}

void WhitelistIpsManager::remove(const QString &hostname)
{
//    if (hash_.contains(hostname))
//        qCDebug(LOG_NETWORK) << "Whitelist ips removed" << hostname;
    hash_.remove(hostname);
    updateIps();
}

void WhitelistIpsManager::updateIps()
{
    QSet<QString> curWhitelistIps;
    for (const auto &it : qAsConst(hash_))
        for (const auto &ip : it)
            curWhitelistIps.insert(ip);

    if (curWhitelistIps != lastWhitelistIps_) {
        lastWhitelistIps_ = curWhitelistIps;
        emit whitelistIpsChanged(lastWhitelistIps_);
    }
}
