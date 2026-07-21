#include "ikev2connectionbase.h"

#include "engine/connectionmanager/iextraconfigaccessor.h"
#include "utils/log/categories.h"
#include "utils/networkingvalidation.h"

Ikev2ConnectionBase::Ikev2ConnectionBase(QObject *parent, types::Protocol protocol, const Ikev2SessionParams &sessionParams)
    : IConnection(parent, protocol), sessionParams_(sessionParams)
{
}

void Ikev2ConnectionBase::prepare(const CurrentConnectionDescr &descr, const AttemptEnvironment &env)
{
    descr_ = descr;
    env_ = env;
    protocol_ = descr.protocol;

    // A domain remoteIp in the ExtraConfig redirects the dial; the rewrite lands in descr_ so
    // effectiveHostname()/effectiveIp() serve the endpoint the firewall whitelist must cover.
    const QString remoteHostname = env_.extraConfig->remoteIp();
    if (NetworkingValidation::isDomain(remoteHostname)) {
        descr_.hostname = remoteHostname;
        descr_.ip = env_.extraConfig->detectedIp();
        qCInfo(LOG_CONNECTION) << "Use data from extra config: hostname=" << descr_.hostname << ", ip=" << descr_.ip
                               << ", remoteId=" << NetworkingValidation::getRemoteIdFromDomain(remoteHostname);
    }

    if (descr_.connectionNodeType == CONNECTION_NODE_STATIC_IPS) {
        username_ = descr_.username;
        password_ = descr_.password;
    } else {
        username_ = sessionParams_.serverCredentials.username();
        password_ = sessionParams_.serverCredentials.password();
    }

    emit prepared();
}
