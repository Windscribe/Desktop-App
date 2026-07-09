#pragma once

#include "iconnectivitydiagnosticcollector.h"

class FirewallController;

// macOS implementation of the local-state snapshot (section A). Reads online
// predicates, default routes, the selected interface and the Windscribe
// firewall state. Synchronous and non-blocking; never logs hosts/ports/credentials.
class ConnectivityDiagnosticCollector_mac : public IConnectivityDiagnosticCollector
{
    Q_OBJECT
public:
    ConnectivityDiagnosticCollector_mac(QObject *parent, FirewallController *firewallController);
    ~ConnectivityDiagnosticCollector_mac() override;

    void collectLocalSnapshot(ConnectivityDiagnosticResult &result) override;

private:
    FirewallController *firewallController_;
};
