#pragma once

#include <QObject>
#include <QTest>

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/helper/helper.h"

class FakeExtraConfig;

// Unit tests for the deterministic parts of OpenVPNConnection::prepare(): MSS calculation, ovpn
// config generation arguments, and the credential pick. Wrapper (stunnel/wstunnel) spawning has no
// seam and stays integration-tested.
class TestOpenVPNConnection : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void testPrepareUdpUsesSessionCredentials();
    void testPrepareRefinesProtocolToDialedVariant();
    void testPrepareStaticIpUsesDescrCredentials();
    void testPrepareManualPacketSizeAddsMss();
    void testPrepareManualPacketSizeHonorsExtraConfigOffset();
    void testPrepareMssTooLowOmitsMssfix();
    void testPrepareAntiCensorshipAddsStuffing();
    void testPrepareCustomConfigRewritesRemoteAndClearsCredentials();
    void testPrepareInvalidNodeIpFails();

private:
    Helper *helper_ = nullptr;
    FakeExtraConfig *extraConfig_ = nullptr;

    AttemptEnvironment makeEnv();
};
