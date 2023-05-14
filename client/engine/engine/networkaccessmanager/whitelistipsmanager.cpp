#include "whitelistipsmanager.h"

WhitelistIpsManager::WhitelistIpsManager(QObject *parent) : QObject(parent)
{
}

void WhitelistIpsManager::add(const QStringList &ips)
{
    bool bChanged = false;
    for (const auto &it : qAsConst(ips)) {
        if (!ips_.contains(it)) {
            ips_.insert(it);
            bChanged = true;
        }
    }
    if (bChanged)
        emit whitelistIpsChanged(ips_);
}

void WhitelistIpsManager::remove(const QStringList &ips)
{
    bool bChanged = false;
    for (const auto &it : qAsConst(ips)) {
        if (ips_.remove(it))
            bChanged = true;
    }
    if (bChanged)
        emit whitelistIpsChanged(ips_);
}
