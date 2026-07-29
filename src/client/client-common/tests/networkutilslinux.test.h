#pragma once

#include <QObject>
#include <QTest>

class TestNetworkUtilsLinux : public QObject
{
    Q_OBJECT

private slots:
    void testIsIpv6EnabledWhenStackAbsent();
    void testIsIpv6EnabledWhenDisabledForAll();
    void testIsIpv6EnabledWhenDisabledForDefault();
    void testIsIpv6EnabledWhenEnabled();
    void testIsIpv6EnabledWhenSysctlsUnreadable();
};
