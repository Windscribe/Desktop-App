#pragma once

#include <QObject>
#include <QTest>

#include "engine/connectionmanager/connectors/iconnection.h"
#include "engine/helper/helper.h"

// Unit tests for the deterministic parts of OpenVPNConnection: prepare() (MSS calculation, ovpn
// config generation arguments, the credential pick), management-reply parsing, management-command
// sanitizing, and MakeOVPNFile generation. Wrapper (stunnel/wstunnel) spawning has no seam and
// stays integration-tested.
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
    void testPrepareCustomConfigInjectsDnsAndClearsCredentials();
    void testPrepareInvalidNodeIpFails();
    void testParsePushReplyRedirectGateway();
    void testParsePushReplyNoRedirectGateway();
    void testParsePushReplyMalformed();
    void testParseDeviceOpenedReply();
    void testParseDeviceOpenedReplyMalformed();
    void testParseConnectedSuccessReply();
    void testParseConnectedSuccessReplyMalformed();
    void testSanitizeString();
    void testMakeOvpnFileStunnelUsesLocalWrapper();
    void testMakeOvpnFileRemoteIpSuppression();
    void testMakeOvpnFileExtraConfigAppend();
    void testMakeOvpnFileTcpBranch();
    void testMakeOvpnFileStunnelNoGatewayNoRoute();
    void testPrepareCustomConfigEmptyDnsNoInjection();
    void testOpenVpnCapabilities();
private:
    Helper *helper_ = nullptr;

    AttemptEnvironment makeEnv();
};
