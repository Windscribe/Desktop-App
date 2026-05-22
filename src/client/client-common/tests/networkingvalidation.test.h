#pragma once

#include <QObject>
#include <QTest>

#include "utils/networkingvalidation.h"

class TestNetworkingValidation : public QObject
{
    Q_OBJECT

private slots:
    void testIsValidMacAddress();
    void testIsValidInterfaceName();
    void testIsValidPort();
    void testIsIpAndIsDomain_smoke();
    void testIsIpv4AndIsIpv6();
    void testIsIpCidr();
    void testIsValidIpForCidr();
    void testIsLocalIp();
    void testIsCtrldCorrectAddress();
};
