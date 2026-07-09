#pragma once

#include <QObject>

struct ConnectivityDiagnosticResult;

// Interface for the platform-specific local-state collection (section A) of the
// kNoApiConnectivity diagnostic. Implementations read only fast, local state and
// fill the section-A fields of the result struct in place.
//
// Contract for implementations:
//  - Synchronous and non-blocking: must NOT perform network I/O. Only fast local
//    queries are allowed (online predicates, default routes, selected interface,
//    proxy mode, Windscribe firewall state).
//  - Fill in place: only the section-A fields of the passed result are written;
//    the orchestrator owns the struct and the async probe sections (B/C).
//  - Unknown vs. false: any value that cannot be determined must be left as the
//    result's default (std::nullopt / empty), distinct from a definite false.
//  - Privacy: never write hosts, ports, credentials, SSIDs, addresses or any
//    other identifying detail — only state bits, modes and adapter type/name.
//
// Inherits QObject so platform collectors participate in Qt parent-child
// ownership (created by the cross-platform factory, torn down with the engine),
// mirroring INetworkDetectionManager.
class IConnectivityDiagnosticCollector : public QObject
{
    Q_OBJECT
public:
    explicit IConnectivityDiagnosticCollector(QObject *parent) : QObject(parent) {}
    ~IConnectivityDiagnosticCollector() override {}

    // Synchronously fills the local-state (section A) portion of the result.
    // See the interface-level contract above. Must return quickly and must not
    // touch the network.
    virtual void collectLocalSnapshot(ConnectivityDiagnosticResult &result) = 0;
};
