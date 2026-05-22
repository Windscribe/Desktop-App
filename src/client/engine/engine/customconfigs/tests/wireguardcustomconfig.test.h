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

    void testEndpointPortZero();
    void testEndpointPortTooLarge();

    void testOversizedFile();
    void testIValueAtLimit();
    void testIValueTooLong();
};
