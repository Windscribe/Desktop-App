#pragma once

#include <QObject>
#include <QTest>

class TestWireguardCustomConfig : public QObject
{
    Q_OBJECT

private slots:
    void testValidWithoutAmneziaParams();
    void testValidHeaderSingleValue();
    void testValidHeaderRange();
    void testValidHeaderRangeMinGreaterThanMax();
    void testInvalidHeaderHex();
    void testInvalidHeaderMultiHyphen();
    void testInvalidHeaderTooManyDigits();

    void testMissingInterfaceSection();
    void testMissingPeerSection();
    void testMissingPrivateKey();
    void testMissingAddress();
    void testMissingDns();
    void testMissingPublicKey();

    void testInvalidBase64PrivateKey();
    void testInvalidBase64PublicKey();
    void testValidPresharedKey();
    void testInvalidPresharedKey();

    void testInvalidAddress();
    void testInvalidAllowedIps();
    void testInvalidDns();

    void testAddressRejectsSmuggledNewline();
    void testDnsRejectsSmuggledNewline();
    void testAllowedIpsRejectsSmuggledNewline();
    void testAddressRejectsTrailingNul();
    void testDnsRejectsTrailingNul();
    void testAddressWhitespaceTolerated();

    void testEndpointPortZero();
    void testEndpointPortTooLarge();

    void testOversizedFile();
    void testIValueAtLimit();
    void testIValueTooLong();

    void testValidAwg3Params();
    void testAwg3UnsetKeysNotEmitted();
    void testHeaderProtectionKeyIsAmneziawg();
    void testHeaderProtectionKeyRequiresPaddings();
    void testInvalidHeaderProtectionKey();
    void testInvalidAwg3Range();
    void testInvalidAwg3Bool();
    void testAwg3RejectsSmuggledNewline();

    void testPersistentKeepaliveDefault();
    void testPersistentKeepaliveValues();
    void testPersistentKeepaliveRangeRequiresAmneziawg();
    void testInvalidPersistentKeepalive();
    void testPersistentKeepaliveAboveUint16();

    void testNoDisconnectOnStaleHandshake();
};
