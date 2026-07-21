#pragma once

#include <QObject>
#include <QTest>

// Unit tests for the deterministic parts of Ikev2ConnectionBase: the ExtraConfig endpoint override
// (domain remoteIp rewrites hostname/ip, served through the effective readbacks) and the
// static-IP vs session credential pick.
class TestIkev2Connection : public QObject
{
    Q_OBJECT

private slots:
    void testCapabilities();
    void testPrepareKeepsEndpointWithoutExtraConfig();
    void testPrepareExtraConfigDomainOverridesEndpoint();
    void testPrepareExtraConfigNonDomainIgnored();
    void testCredentialPickSessionForDefaultNode();
    void testCredentialPickDescrForStaticIpNode();
};
