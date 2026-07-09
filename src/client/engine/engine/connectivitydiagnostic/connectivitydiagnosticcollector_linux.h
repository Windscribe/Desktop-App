#pragma once

#include "iconnectivitydiagnosticcollector.h"

class FirewallController;

// Linux implementation of the local-state snapshot (section A). Reads online
// predicates, default routes, the selected interface and the Windscribe
// firewall state. Synchronous and non-blocking; never logs hosts/ports/credentials.
class ConnectivityDiagnosticCollector_linux : public IConnectivityDiagnosticCollector
{
    Q_OBJECT
public:
    ConnectivityDiagnosticCollector_linux(QObject *parent, FirewallController *firewallController);
    ~ConnectivityDiagnosticCollector_linux() override;

    void collectLocalSnapshot(ConnectivityDiagnosticResult &result) override;

private:
    FirewallController *firewallController_;
};
