#pragma once

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/connectionmanager/connectors/wireguard/wireguardsessionparams.h"
#include "engine/wireguardconfig/wireguardconfig.h"

class GetWireGuardConfig;
enum class WireGuardConfigRetCode;

// Platform-independent WireGuard preparation: config fetch (or custom-config intake), dual-stack /
// IPv4-only strip, endpoint validation and assembly, and the key-limit consent resume. Platform
// classes derive from this and dial with config().
class WireGuardConnectionBase : public IConnection
{
    Q_OBJECT

public:
    WireGuardConnectionBase(QObject *parent, types::Protocol protocol, const WireGuardSessionParams &sessionParams);

    void prepare(const CurrentConnectionDescr &descr, const AttemptEnvironment &env) override;
    void teardown() override;
    void continueWithUserInput(const UserInputResponse &response) override;

    // Pre-override tunnel DNS: the fetched/custom config's own DNS, before any custom-DNS rewrite
    // the dial applies. Never null — a null readback would tell the manager the config carries no
    // DNS of its own, skipping the firewall whitelist of the connected-DNS override even though
    // dialConfig() still applies it (reachable when an IPv4-only strip removes a v6-only DNS entry).
    QString tunnelDefaultDns() const override
    {
        const QString dns = config_.clientDnsAddress();
        return dns.isNull() ? QString("") : dns;
    }

protected:
    const WireGuardConfig &config() const { return config_; }
    // config() with the attempt environment's DNS override applied for the dial; config_ stays
    // pristine so tunnelDefaultDns() keeps reporting the config's own DNS.
    WireGuardConfig dialConfig() const;

private slots:
    void onGetWireGuardConfigAnswer(WireGuardConfigRetCode retCode, const WireGuardConfig &config);

private:
#ifdef WINDSCRIBE_BUILD_TESTS
    friend class TestWireGuardConnection;
#endif

    void fetchConfig(bool deleteOldestKey);

    WireGuardSessionParams sessionParams_;
    WireGuardConfig config_;
    GetWireGuardConfig *getWireGuardConfig_ = nullptr;
};
