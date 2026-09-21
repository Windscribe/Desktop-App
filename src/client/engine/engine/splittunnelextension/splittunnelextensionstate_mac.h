#pragma once

#import <NetworkExtension/NetworkExtension.h>

#include "splittunnelextensionmanager_mac.h"

struct SplitTunnelExtensionManager::State
{
    NETransparentProxyManager *proxyManager = nil; // cached; nil until setupManager succeeds
    id statusObserver = nil;  // persistent session status observer; drives reconcile()
    bool setupInFlight = false;
    bool wantStarted = false; // desired: a proxy session should be up
    bool startIssued = false; // startTunnel succeeded and the session has not been seen down since;
                              // session.status updates asynchronously, so this guards double-starts
    bool sessionFailed = false;
    quint64 failureGeneration = 0;
    bool dirty = false;       // settings/interfaces changed since the session last started; restart
    bool isExclude = false;
    QString primaryInterface;
    QString vpnInterface;
    QStringList appPaths;
    QStringList ips;
    QStringList hostnames;
};
