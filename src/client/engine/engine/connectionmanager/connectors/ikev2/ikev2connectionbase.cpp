#include "ikev2connectionbase.h"

#include "utils/extraconfig.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"

Ikev2ConnectionBase::Ikev2ConnectionBase(QObject *parent, types::Protocol protocol, const Ikev2SessionParams &sessionParams)
    : IConnection(parent, protocol), sessionParams_(sessionParams)
{
}

void Ikev2ConnectionBase::prepareImpl()
{
    // A domain remoteIp in the ExtraConfig redirects the dial; the rewrite lands in descr_ so
    // effectiveHostname()/effectiveIp() serve the endpoint the firewall whitelist must cover.
    const QString remoteHostname = ExtraConfig::instance().getRemoteIpFromExtraConfig();
    if (NetworkingValidation::isDomain(remoteHostname)) {
        descr_.hostname = remoteHostname;
        descr_.ip = ExtraConfig::instance().getDetectedIp();
        qCInfo(LOG_CONNECTION) << "Use data from extra config: hostname=" << descr_.hostname << ", ip=" << descr_.ip
                               << ", remoteId=" << NetworkingValidation::getRemoteIdFromDomain(remoteHostname);
    }

    if (descr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS) {
        username_ = descr_.staticIps.credentials.username();
        password_ = descr_.staticIps.credentials.password();
    } else {
        username_ = sessionParams_.serverCredentials.username();
        password_ = sessionParams_.serverCredentials.password();
    }

    emit prepared();
}
