#pragma once

#include <QObject>

class TestCustomConfigLocationInfo : public QObject
{
    Q_OBJECT

private slots:
    void testSelectedEndpointConfigAppendsRemoteToBody();
    void testSelectedEndpointConfigSubstitutesResolvedIp();
    void testSelectedEndpointConfigFollowsSelectedNode();
    void testIsExistSelectedNodeFalseCases();
    void testResolveHostnamesReEmitsWhenAlreadyResolved();
    void testSelectedPortProtocolGlobalFallback();
    void testOvpnV6LiteralRemoteSkipped();
    void testWireGuardIpLiteralResolve();
    void testWireGuardV6LiteralEndpointSkipped();
    void testHostnameMultiIpRotation();
};
